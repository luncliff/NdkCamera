#include <magic/coroutine.hpp>
#include <camera/NdkCameraMetadata.h>

#include "adapter.h"

#define PROLOGUE __attribute__((constructor))
#define EPILOGUE __attribute__((destructor))

using NativeWindow = std::unique_ptr<ANativeWindow,
                                     void (*)(ANativeWindow *)>;
using CaptureSessionOutputContainer = std::unique_ptr<ACaptureSessionOutputContainer,
                                                      void (*)(ACaptureSessionOutputContainer *)>;
using CaptureSessionOutput = std::unique_ptr<ACaptureSessionOutput,
                                             void (*)(ACaptureSessionOutput *)>;
using CaptureRequest = std::unique_ptr<ACaptureRequest,
                                       void (*)(ACaptureRequest *)>;

using CameraOutputTarget = std::unique_ptr<ACameraOutputTarget,
                                           void (*)(ACameraOutputTarget *)>;

constexpr auto tag = "ndk_camera";

std::shared_ptr<spdlog::logger> logger{};

namespace java
{
// TBA...
}

jclass illegal_argument_exception{};
jclass runtime_exception{};
jclass device_java_t{};
jfieldID device_id_f{};

/**
 * Library context. Supports auto releasing and facade for features
 */
struct context_t
{
    // without external camera, 2 is enough(back + front).
    // But we will use more since there might be multiple(for now, 2) external camera...
    static constexpr auto max_camera_count = 4;

  public:
    ACameraManager *manager = nullptr;
    ACameraIdList *id_list = nullptr;
    //    ACaptureSessionOutputContainer* container = nullptr;
    //    ACaptureSessionOutput* output = nullptr;

    // cached metadata
    std::array<ACameraMetadata *, max_camera_count> metadata_set{};

    // even though android system limits the number of maximum open camera device,
    // we will consider multiple camera are working concurrently.
    //
    // if element is nullptr, it means the device is not open.
    std::array<ACameraDevice *, max_camera_count> device_set{};

    std::array<ACameraCaptureSession *, max_camera_count> session_set{};

    // sequence number from capture session
    std::array<int, max_camera_count> seq_id_set{};

  public:
    context_t() noexcept = default;
    context_t(const context_t &) = delete;
    context_t(context_t &&) = delete;
    context_t &operator=(const context_t &) = delete;
    context_t &operator=(context_t &&) = delete;
    ~context_t() noexcept
    {
        this->release(); // ensure release precedure
    }

  public:
    void release() noexcept
    {
        for (auto &session : session_set)
            if (session)
            {
                ACameraCaptureSession_close(session);
                session = nullptr;
            }

        for (auto &device : device_set)
            if (device)
            {
                ACameraDevice_close(device);
                device = nullptr;
            }

        for (auto &meta : metadata_set)
            if (meta)
            {
                ACameraMetadata_free(meta);
                meta = nullptr;
            }

        if (id_list)
            ACameraManager_deleteCameraIdList(id_list);
        id_list = nullptr;

        if (manager)
            ACameraManager_delete(manager);
        manager = nullptr;
    }

  public:
    camera_status_t open_device(uint16_t id,
                                ACameraDevice_StateCallbacks &callbacks) noexcept
    {
        auto &device = this->device_set[id];
        auto status =
            ACameraManager_openCamera(
                this->manager, this->id_list->cameraIds[id], std::addressof(callbacks),
                std::addressof(device));
        return status;
    }

    // Notice that this routine doesn't free metadata
    void close_device(uint16_t id) noexcept
    {
        auto &device = this->device_set[id];
        if (device)
            ACameraDevice_close(device); // forced close

        device = nullptr;
    }

    camera_status_t start_repeat(
        uint16_t id,
        ANativeWindow *window,
        ACameraCaptureSession_stateCallbacks &on_session_changed,
        ACameraCaptureSession_captureCallbacks &on_capture_event) noexcept
    {
        camera_status_t status = ACAMERA_OK;

        // ---- target surface for camera ----
        auto target = CameraOutputTarget{
            [=]() {
                ACameraOutputTarget *target{};
                ACameraOutputTarget_create(window, std::addressof(target));
                return target;
            }(),
            ACameraOutputTarget_free};

        // ---- capture request (preview) ----
        auto request = CaptureRequest{
            [=]() {
                ACaptureRequest *ptr{};
                // capture as a preview
                // TEMPLATE_RECORD, TEMPLATE_PREVIEW, TEMPLATE_MANUAL,
                const auto status =
                    ACameraDevice_createCaptureRequest(
                        this->device_set[id], TEMPLATE_PREVIEW,
                        &ptr);
                assert(status == ACAMERA_OK);
                return ptr;
            }(),
            ACaptureRequest_free};

        // `ACaptureRequest` == how to capture
        // detailed config comes here...
        // ACaptureRequest_setEntry_*
        // - ACAMERA_REQUEST_MAX_NUM_OUTPUT_STREAMS
        // -

        // designate target surface in request
        ACaptureRequest_addTarget(request.get(),
                                  target.get());

        // ---- session output ----

        // container for multiplexing of session output
        auto container = CaptureSessionOutputContainer{
            // return container
            []() {ACaptureSessionOutputContainer * container{};
                    ACaptureSessionOutputContainer_create(&container);
                    return container; }(),
            // free container
            ACaptureSessionOutputContainer_free};

        // session output
        auto output = CaptureSessionOutput{
            [=]() {
                ACaptureSessionOutput *output{};
                ACaptureSessionOutput_create(window, &output);
                return output;
            }(),
            ACaptureSessionOutput_free};

        ACaptureSessionOutputContainer_add(container.get(),
                                           output.get());

        // ---- create a session ----
        status = ACameraDevice_createCaptureSession(
            this->device_set[id], container.get(),
            std::addressof(on_session_changed),
            std::addressof(this->session_set[id]));
        assert(status == ACAMERA_OK);

        // ---- set request ----
        ACaptureRequest *batch_request[1]{};
        batch_request[0] = request.get();

        status = ACameraCaptureSession_setRepeatingRequest(
            this->session_set[id],
            std::addressof(on_capture_event), 1, batch_request,
            std::addressof(this->seq_id_set[id]));
        assert(status == ACAMERA_OK);

        ACaptureRequest_removeTarget(request.get(),
                                     target.get());

        return status;
    }

    void stop_repeat(uint16_t id) noexcept
    {
        auto &session = this->session_set[id];
        if (session)
        {
            // ACameraCaptureSession_setRepeatingRequest
            ACameraCaptureSession_stopRepeating(session);
            ACameraCaptureSession_close(session);
            session = nullptr;
        }
        this->seq_id_set[id] = 0;
    }

    camera_status_t start_capture(
        uint16_t id,
        ANativeWindow *window,
        ACameraCaptureSession_stateCallbacks &on_session_changed,
        ACameraCaptureSession_captureCallbacks &on_capture_event) noexcept
    {
        camera_status_t status = ACAMERA_OK;

        // ---- target surface for camera ----
        auto target = CameraOutputTarget{
            [=]() {
                ACameraOutputTarget *target{};
                ACameraOutputTarget_create(window, std::addressof(target));
                return target;
            }(),
            ACameraOutputTarget_free};

        // ---- capture request (preview) ----
        auto request = CaptureRequest{
            [=]() {
                ACaptureRequest *ptr{};
                // capture as a preview
                // TEMPLATE_RECORD, TEMPLATE_PREVIEW, TEMPLATE_MANUAL,
                const auto status =
                    ACameraDevice_createCaptureRequest(
                        this->device_set[id], TEMPLATE_STILL_CAPTURE,
                        &ptr);
                assert(status == ACAMERA_OK);
                return ptr;
            }(),
            ACaptureRequest_free};

        // `ACaptureRequest` == how to capture
        // detailed config comes here...
        // ACaptureRequest_setEntry_*
        // - ACAMERA_REQUEST_MAX_NUM_OUTPUT_STREAMS
        // -

        // designate target surface in request
        ACaptureRequest_addTarget(request.get(),
                                  target.get());
        // defer    ACaptureRequest_removeTarget;

        // ---- session output ----

        // container for multiplexing of session output
        auto container = CaptureSessionOutputContainer{
            // return container
            []() {ACaptureSessionOutputContainer * container{};
                    ACaptureSessionOutputContainer_create(&container);
                    return container; }(),
            // free container
            ACaptureSessionOutputContainer_free};

        // session output
        auto output = CaptureSessionOutput{
            [=]() {
                ACaptureSessionOutput *output{};
                ACaptureSessionOutput_create(window, &output);
                return output;
            }(),
            ACaptureSessionOutput_free};

        ACaptureSessionOutputContainer_add(container.get(),
                                           output.get());

        // ---- create a session ----
        status = ACameraDevice_createCaptureSession(
            this->device_set[id], container.get(),
            std::addressof(on_session_changed),
            std::addressof(this->session_set[id]));
        assert(status == ACAMERA_OK);

        // ---- set request ----
        ACaptureRequest *batch_request[1]{};
        batch_request[0] = request.get();

        status = ACameraCaptureSession_capture(
            this->session_set[id],
            std::addressof(on_capture_event), 1, batch_request,
            std::addressof(this->seq_id_set[id]));
        assert(status == ACAMERA_OK);

        ACaptureRequest_removeTarget(request.get(),
                                     target.get());

        return status;
    }

    void stop_capture(uint16_t id) noexcept
    {
        auto &session = this->session_set[id];
        if (session)
        {
            ACameraCaptureSession_abortCaptures(session);
            ACameraCaptureSession_close(session);
            session = nullptr;
        }
        this->seq_id_set[id] = 0;
    }

    // ACAMERA_LENS_FACING_FRONT
    // ACAMERA_LENS_FACING_BACK
    // ACAMERA_LENS_FACING_EXTERNAL
    uint16_t get_facing(uint16_t id) noexcept
    {
        // const ACameraMetadata*
        const auto *metadata = metadata_set[id];

        ACameraMetadata_const_entry lens_facing{};
        ACameraMetadata_getConstEntry(metadata,
                                      ACAMERA_LENS_FACING, &lens_facing);

        const auto *ptr = lens_facing.data.u8;
        return *ptr;
    }
};

void context_on_device_disconnected(context_t &context,
                                    ACameraDevice *device) noexcept
{
    const char *id = ACameraDevice_getId(device);
    logger->error("on_device_disconnect: {}", id);
    return;
}

void context_on_device_error(context_t &context,
                             ACameraDevice *device, int error) noexcept
{
    const char *id = ACameraDevice_getId(device);
    logger->error("on_device_error: {} {}", id, error);
    return;
}

void context_on_session_active(context_t &context,
                               ACameraCaptureSession *session) noexcept
{
    logger->info("on_session_active");
    return;
}

void context_on_session_closed(context_t &context,
                               ACameraCaptureSession *session) noexcept
{
    logger->warn("on_session_closed");
    return;
}

void context_on_session_ready(context_t &context,
                              ACameraCaptureSession *session) noexcept
{
    logger->info("on_session_ready");
    return;
}

void context_on_capture_started(context_t &context, ACameraCaptureSession *session,
                                const ACaptureRequest *request,
                                uint64_t time_point) noexcept
{
    logger->info("context_on_capture_started  : {}", time_point);
    return;
}

void context_on_capture_progressed(context_t &context, ACameraCaptureSession *session,
                                   ACaptureRequest *request,
                                   const ACameraMetadata *result) noexcept
{
    camera_status_t status = ACAMERA_OK;
    ACameraMetadata_const_entry entry{};
    uint64_t time_point = 0;
    // ACAMERA_SENSOR_TIMESTAMP
    // ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE
    // ACAMERA_SENSOR_FRAME_DURATION
    status = ACameraMetadata_getConstEntry(result, ACAMERA_SENSOR_TIMESTAMP, &entry);
    if (status == ACAMERA_OK)
        time_point = static_cast<uint64_t>(*(entry.data.i64));

    logger->debug("context_on_capture_progressed: {}", time_point);
    return;
}
void context_on_capture_completed(context_t &context, ACameraCaptureSession *session,
                                  ACaptureRequest *request,
                                  const ACameraMetadata *result) noexcept
{
    camera_status_t status = ACAMERA_OK;
    ACameraMetadata_const_entry entry{};
    uint64_t time_point = 0;
    // ACAMERA_SENSOR_TIMESTAMP
    // ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE
    // ACAMERA_SENSOR_FRAME_DURATION
    status = ACameraMetadata_getConstEntry(result, ACAMERA_SENSOR_TIMESTAMP, &entry);
    if (status == ACAMERA_OK)
        time_point = static_cast<uint64_t>(*(entry.data.i64));

    logger->debug("context_on_capture_completed: {}", time_point);
    return;
}

void context_on_capture_failed(context_t &context, ACameraCaptureSession *session,
                               ACaptureRequest *request,
                               ACameraCaptureFailure *failure) noexcept
{
    logger->error("context_on_capture_failed {} {} {} {}",
                  failure->frameNumber,
                  failure->reason,
                  failure->sequenceId,
                  failure->wasImageCaptured);
    return;
}
void context_on_capture_buffer_lost(context_t &context, ACameraCaptureSession *session,
                                    ACaptureRequest *request,
                                    ANativeWindow *window, int64_t frameNumber) noexcept
{
    logger->error("context_on_capture_buffer_lost");
    return;
}

void context_on_capture_sequence_abort(context_t &context, ACameraCaptureSession *session,
                                       int sequenceId) noexcept
{
    logger->error("context_on_capture_sequence_abort");
    return;
}

void context_on_capture_sequence_complete(context_t &context, ACameraCaptureSession *session,
                                          int sequenceId, int64_t frameNumber) noexcept
{
    logger->error("context_on_capture_sequence_complete");
    return;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

context_t context{};

void Java_ndcam_CameraModel_Init(JNIEnv *env, jclass type) noexcept
{
    // already initialized. do nothing
    if (context.manager != nullptr)
        return;

    camera_status_t status = ACAMERA_OK;
    assert(logger != nullptr);

    // Find exception class information (type info)
    illegal_argument_exception = env->FindClass(
        "java/lang/IllegalArgumentException");
    runtime_exception = env->FindClass(
        "java/lang/RuntimeException");

    // !!! Since we can't throw if this info is null, call assert !!!
    assert(illegal_argument_exception != nullptr);
    assert(runtime_exception != nullptr);

    context.release();

    // auto managerHolder = CameraManager{ACameraManager_create(), ACameraManager_delete};
    context.manager = ACameraManager_create();
    assert(context.manager != nullptr);

    status = ACameraManager_getCameraIdList(context.manager,
                                            &context.id_list);
    if (status != ACAMERA_OK)
        goto ThrowJavaException;

    // https://developer.android.com/reference/android/hardware/camera2/CameraMetadata
    // https://android.googlesource.com/platform/frameworks/av/+/2e19c3c/services/camera/libcameraservice/camera2/CameraMetadata.h
    for (uint16_t i = 0u; i < context.id_list->numCameras; ++i)
    {
        status = ACameraManager_getCameraCharacteristics(
            context.manager, context.id_list->cameraIds[i],
            std::addressof(context.metadata_set[i]));

        if (status == ACAMERA_OK)
            continue;

        logger->error("ACameraManager_getCameraCharacteristics");
        goto ThrowJavaException;
    }
    return;
ThrowJavaException:
    env->ThrowNew(illegal_argument_exception, camera_error_message(status));
}

jint Java_ndcam_CameraModel_GetDeviceCount(JNIEnv *env, jclass type) noexcept
{
    if (context.manager == nullptr) // not initialized
        return 0;

    return context.id_list->numCameras;
}

void Java_ndcam_CameraModel_SetDeviceData(JNIEnv *env, jclass type,
                                          jobjectArray devices) noexcept
{
    if (context.manager == nullptr) // not initialized
        return;

    device_java_t = env->FindClass("ndcam/Device");
    assert(device_java_t != nullptr);

    const auto count = context.id_list->numCameras;
    assert(count == env->GetArrayLength(devices));

    for (short index = 0; index < count; ++index)
    {
        jobject device = env->GetObjectArrayElement(devices, index);
        assert(device != nullptr);

        device_id_f = env->GetFieldID(device_java_t, "id", "S"); // short
        assert(device_id_f != nullptr);

        env->SetShortField(device, device_id_f, index);
    }
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

jbyte Java_ndcam_Device_facing(JNIEnv *env, jobject instance) noexcept
{
    if (context.manager == nullptr) // not initialized
        return JNI_FALSE;

    auto device_id = env->GetShortField(instance, device_id_f);
    assert(device_id != -1);

    const auto facing = context.get_facing(static_cast<uint16_t>(device_id));
    return static_cast<jbyte>(facing);
}

void Java_ndcam_Device_open(JNIEnv *env, jobject instance) noexcept
{
    camera_status_t status = ACAMERA_OK;
    if (context.manager == nullptr) // not initialized
        return;

    const auto id = env->GetShortField(instance, device_id_f);
    assert(id != -1);

    ACameraDevice_StateCallbacks callbacks{};
    callbacks.context = std::addressof(context);
    callbacks.onDisconnected = reinterpret_cast<ACameraDevice_StateCallback>(context_on_device_disconnected);
    callbacks.onError = reinterpret_cast<ACameraDevice_ErrorStateCallback>(context_on_device_error);

    context.close_device(id);
    status = context.open_device(id, callbacks);

    if (status == ACAMERA_OK)
        return;

    // throw exception
    env->ThrowNew(runtime_exception,
                  fmt::format("ACameraManager_openCamera: {}", status).c_str());
}

void Java_ndcam_Device_close(JNIEnv *env, jobject instance) noexcept
{
    if (context.manager == nullptr) // not initialized
        return;

    const auto id = env->GetShortField(instance, device_id_f);
    assert(id != -1);

    context.close_device(id);
}

// TODO: Resource cleanup
void Java_ndcam_Device_startRepeat(JNIEnv *env, jobject instance,
                                   jobject surface) noexcept
{
    camera_status_t status = ACAMERA_OK;
    if (context.manager == nullptr) // not initialized
        return;
    const auto id = env->GetShortField(instance, device_id_f);
    assert(id != -1);

    // `ANativeWindow_fromSurface` acquires a reference
    // `ANativeWindow_release` releases it
    // ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    auto window = NativeWindow{ANativeWindow_fromSurface(env, surface),
                               ANativeWindow_release};

    ACameraCaptureSession_stateCallbacks on_state_changed{};
    on_state_changed.context = std::addressof(context);
    on_state_changed.onReady = reinterpret_cast<ACameraCaptureSession_stateCallback>(context_on_session_ready);
    on_state_changed.onClosed = reinterpret_cast<ACameraCaptureSession_stateCallback>(context_on_session_closed);
    on_state_changed.onActive = reinterpret_cast<ACameraCaptureSession_stateCallback>(context_on_session_active);

    ACameraCaptureSession_captureCallbacks on_capture_event{};
    on_capture_event.context = std::addressof(context);
    on_capture_event.onCaptureStarted =
        reinterpret_cast<ACameraCaptureSession_captureCallback_start>(
            context_on_capture_started);
    on_capture_event.onCaptureBufferLost =
        reinterpret_cast<ACameraCaptureSession_captureCallback_bufferLost>(
            context_on_capture_buffer_lost);
    on_capture_event.onCaptureProgressed =
        reinterpret_cast<ACameraCaptureSession_captureCallback_result>(
            context_on_capture_progressed);
    on_capture_event.onCaptureCompleted =
        reinterpret_cast<ACameraCaptureSession_captureCallback_result>(
            context_on_capture_completed);
    on_capture_event.onCaptureFailed =
        reinterpret_cast<ACameraCaptureSession_captureCallback_failed>(
            context_on_capture_failed);
    on_capture_event.onCaptureSequenceAborted =
        reinterpret_cast<ACameraCaptureSession_captureCallback_sequenceAbort>(
            context_on_capture_sequence_abort);
    on_capture_event.onCaptureSequenceCompleted =
        reinterpret_cast<ACameraCaptureSession_captureCallback_sequenceEnd>(
            context_on_capture_sequence_complete);

    status = context.start_repeat(id, window.get(),
                                  on_state_changed, on_capture_event);
    assert(status == ACAMERA_OK);

    //    context.request_capture(id, window.get(), on_capture_event);
    //    context.stop_capture(id);
}

void Java_ndcam_Device_stopRepeat(JNIEnv *env, jobject instance) noexcept
{
    if (context.manager == nullptr) // not initialized
        return;

    const auto id = env->GetShortField(instance, device_id_f);
    assert(id != -1);

    context.stop_repeat(id);
}

void Java_ndcam_Device_startCapture(JNIEnv *env, jobject instance,
                                    jobject surface) noexcept
{
    camera_status_t status = ACAMERA_OK;
    if (context.manager == nullptr) // not initialized
        return;

    auto id = env->GetShortField(instance, device_id_f);
    assert(id != -1);

    // `ANativeWindow_fromSurface` acquires a reference
    // `ANativeWindow_release` releases it
    // ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    auto window = NativeWindow{ANativeWindow_fromSurface(env, surface),
                               ANativeWindow_release};

    ACameraCaptureSession_stateCallbacks on_state_changed{};
    on_state_changed.context = std::addressof(context);
    on_state_changed.onReady = reinterpret_cast<ACameraCaptureSession_stateCallback>(context_on_session_ready);
    on_state_changed.onClosed = reinterpret_cast<ACameraCaptureSession_stateCallback>(context_on_session_closed);
    on_state_changed.onActive = reinterpret_cast<ACameraCaptureSession_stateCallback>(context_on_session_active);

    ACameraCaptureSession_captureCallbacks on_capture_event{};
    on_capture_event.context = std::addressof(context);
    on_capture_event.onCaptureStarted =
        reinterpret_cast<ACameraCaptureSession_captureCallback_start>(
            context_on_capture_started);
    on_capture_event.onCaptureBufferLost =
        reinterpret_cast<ACameraCaptureSession_captureCallback_bufferLost>(
            context_on_capture_buffer_lost);
    on_capture_event.onCaptureProgressed =
        reinterpret_cast<ACameraCaptureSession_captureCallback_result>(
            context_on_capture_progressed);
    on_capture_event.onCaptureCompleted =
        reinterpret_cast<ACameraCaptureSession_captureCallback_result>(
            context_on_capture_completed);
    on_capture_event.onCaptureFailed =
        reinterpret_cast<ACameraCaptureSession_captureCallback_failed>(
            context_on_capture_failed);
    on_capture_event.onCaptureSequenceAborted =
        reinterpret_cast<ACameraCaptureSession_captureCallback_sequenceAbort>(
            context_on_capture_sequence_abort);
    on_capture_event.onCaptureSequenceCompleted =
        reinterpret_cast<ACameraCaptureSession_captureCallback_sequenceEnd>(
            context_on_capture_sequence_complete);

    status = context.start_capture(id, window.get(),
                                   on_state_changed, on_capture_event);
    assert(status == ACAMERA_OK);
}

void Java_ndcam_Device_stopCapture(JNIEnv *env, jobject instance) noexcept
{
    if (context.manager == nullptr) // not initialized
        return;

    const auto id = env->GetShortField(instance, device_id_f);
    assert(id != -1);

    context.stop_capture(id);
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

// https://github.com/gabime/spdlog
PROLOGUE void jni_on_load(void) noexcept
{
    auto check_coroutine_available = [=]() -> magic::unplug {
        co_await magic::stdex::suspend_never{};
        // see fmt::format for usage
        logger->info("{}.{}", tag, nullptr);
    };

    // log will print thread id and message
    spdlog::set_pattern("[thread %t] %v");
    logger = spdlog::android_logger_st("android", tag);

    check_coroutine_available();
}

/**
 * @see NdkCameraError.h
 */
auto camera_error_message(camera_status_t status) noexcept -> const char *
{
    switch (status)
    {
    case ACAMERA_ERROR_UNKNOWN:
        return "Camera operation has failed due to an unspecified cause.";
    case ACAMERA_ERROR_INVALID_PARAMETER:
        return "Camera operation has failed due to an invalid parameter being passed to the method.";
    case ACAMERA_ERROR_CAMERA_DISCONNECTED:
        return "Camera operation has failed because the camera device has been closed, possibly because a higher-priority client has taken ownership of the camera device.";
    case ACAMERA_ERROR_NOT_ENOUGH_MEMORY:
        return "Camera operation has failed due to insufficient memory.";
    case ACAMERA_ERROR_METADATA_NOT_FOUND:
        return "Camera operation has failed due to the requested metadata tag cannot be found in input. ACameraMetadata or ACaptureRequest";
    case ACAMERA_ERROR_CAMERA_DEVICE:
        return "Camera operation has failed and the camera device has encountered a fatal error and needs to be re-opened before it can be used again.";
    case ACAMERA_ERROR_CAMERA_SERVICE:
        /**
         * Camera operation has failed and the camera service has encountered a fatal error.
         *
         * <p>The Android device may need to be shut down and restarted to restore
         * camera function, or there may be a persistent hardware problem.</p>
         *
         * <p>An attempt at recovery may be possible by closing the
         * ACameraDevice and the ACameraManager, and trying to acquire all resources
         * again from scratch.</p>
         */
        return "Camera operation has failed and the camera service has encountered a fatal error.";
    case ACAMERA_ERROR_SESSION_CLOSED:
        return "The ACameraCaptureSession has been closed and cannot perform any operation other than ACameraCaptureSession_close.";
    case ACAMERA_ERROR_INVALID_OPERATION:
        return "Camera operation has failed due to an invalid internal operation. Usually this is due to a low-level problem that may resolve itself on retry";
    case ACAMERA_ERROR_STREAM_CONFIGURE_FAIL:
        return "Camera device does not support the stream configuration provided by application in ACameraDevice_createCaptureSession.";
    case ACAMERA_ERROR_CAMERA_IN_USE:
        return "Camera device is being used by another higher priority camera API client.";
    case ACAMERA_ERROR_MAX_CAMERA_IN_USE:
        return "The system-wide limit for number of open cameras or camera resources has been reached, and more camera devices cannot be opened until previous instances are closed.";
    case ACAMERA_ERROR_CAMERA_DISABLED:
        return "The camera is disabled due to a device policy, and cannot be opened.";
    case ACAMERA_ERROR_PERMISSION_DENIED:
        return "The application does not have permission to open camera.";
    default:
        return "ACAMERA_OK";
    }
}
