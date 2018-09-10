#include <magic/coroutine.hpp>
#include <camera/NdkCameraMetadata.h>

#include "adapter.h"

#define PROLOGUE __attribute__((constructor))
#define EPILOGUE __attribute__((destructor))

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
    ~context_t() noexcept
    {
        this->release();
    }

    void release() noexcept
    {
        if (manager)
            ACameraManager_delete(manager);
        manager = nullptr;

        if (id_list)
            ACameraManager_deleteCameraIdList(id_list);
        id_list = nullptr;

        for (auto &meta : metadata_set)
            if (meta)
            {
                ACameraMetadata_free(meta);
                meta = nullptr;
            }

        for (auto &device : device_set)
            if (device)
            {
                ACameraDevice_close(device);
                device = nullptr;
            }
    }

    int32_t get_facing(uint16_t id) noexcept
    {
        // const ACameraMetadata*
        const auto *metadata = metadata_set[id];

        ACameraMetadata_const_entry lens_facing{};
        ACameraMetadata_getConstEntry(metadata,
                                      ACAMERA_LENS_FACING, &lens_facing);

        const auto *ptr = lens_facing.data.i32;
        return *ptr;
    }
};

void context_on_device_disconnected(context_t &context, ACameraDevice *device) noexcept
{
    const auto id = ACameraDevice_getId(device);
    logger->error("on_device_disconnect: {}", id);
    return;
}

void context_on_device_error(context_t &context, ACameraDevice *device, int error) noexcept
{
    const auto id = ACameraDevice_getId(device);
    logger->error("on_device_error: {} {}", id, error);
    return;
}

void context_on_session_active(context_t &context, ACameraCaptureSession *session) noexcept
{
    logger->error("on_session_active");
    return;
}

void context_on_session_closed(context_t &context, ACameraCaptureSession *session) noexcept
{
    logger->error("on_session_closed");
    return;
}
void context_on_session_ready(context_t &context, ACameraCaptureSession *session) noexcept
{
    logger->error("on_session_ready");
    return;
}

void context_on_capture_started(context_t &context, ACameraCaptureSession *session,
                                const ACaptureRequest *request,
                                int64_t timestamp) noexcept
{
    logger->error("context_on_capture_started");
    return;
}

void context_on_capture_completed(context_t &context, ACameraCaptureSession *session,
                                  ACaptureRequest *request,
                                  const ACameraMetadata *result) noexcept
{
    logger->error("context_on_capture_completed");
    return;
}

void context_on_capture_failed(context_t &context, ACameraCaptureSession *session,
                               ACaptureRequest *request,
                               ACameraCaptureFailure *failure) noexcept
{
    logger->error("context_on_capture_failed");
    return;
}

context_t context{};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

void Java_ndcam_CameraModel_Init(JNIEnv *env, jclass type) noexcept
{
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
    logger->warn("ndk_camera is under develop");

    return;
ThrowJavaException:
    env->ThrowNew(illegal_argument_exception, error_status_message(status));
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

void Java_ndcam_Device_open(JNIEnv *env, jobject instance) noexcept
{
    camera_status_t status = ACAMERA_OK;
    if (context.manager == nullptr) // not initialized
        return;

    const auto id = env->GetShortField(instance, device_id_f);
    assert(id != -1);

    auto &device = context.device_set[id];
    if (device != nullptr)
        ACameraDevice_close(device);

    ACameraDevice_StateCallbacks callbacks{};
    callbacks.context = std::addressof(context);
    callbacks.onDisconnected = reinterpret_cast<ACameraDevice_StateCallback>(context_on_device_disconnected);
    callbacks.onError = reinterpret_cast<ACameraDevice_ErrorStateCallback>(context_on_device_error);
    status = ACameraManager_openCamera(context.manager, context.id_list->cameraIds[id], &callbacks,
                                       std::addressof(context.device_set[id]));
    if (status != ACAMERA_OK)
        goto ThrowJavaException;

    return;
ThrowJavaException:
    env->ThrowNew(runtime_exception, fmt::format("ACameraManager_openCamera: {}", status).c_str());
}

jboolean
Java_ndcam_Device_isBack(JNIEnv *env, jobject instance) noexcept
{
    if (context.manager == nullptr) // not initialized
        return JNI_FALSE;

    auto device_id = env->GetShortField(instance, device_id_f);
    assert(device_id != -1);

    const auto facing = context.get_facing(static_cast<uint16_t>(device_id));
    return static_cast<jboolean>(facing == ACAMERA_LENS_FACING_BACK);
}

jboolean
Java_ndcam_Device_isExternal(JNIEnv *env, jobject instance) noexcept
{
    auto device_id = env->GetShortField(instance, device_id_f);
    assert(device_id != -1);

    const auto facing = context.get_facing(static_cast<uint16_t>(device_id));
    return static_cast<jboolean>(facing == ACAMERA_LENS_FACING_EXTERNAL);
}

jboolean
Java_ndcam_Device_isFront(JNIEnv *env, jobject instance) noexcept
{
    auto device_id = env->GetShortField(instance, device_id_f);
    assert(device_id != -1);

    const auto facing = context.get_facing(static_cast<uint16_t>(device_id));
    return static_cast<jboolean>(facing == ACAMERA_LENS_FACING_FRONT);
}

void Java_ndcam_Device_startRepeat(JNIEnv *env, jobject instance,
                                   jobject surface) noexcept
{
    camera_status_t status = ACAMERA_OK;
    if (context.manager == nullptr) // not initialized
        return;
    const auto id = env->GetShortField(instance, device_id_f);
    assert(id != -1);

    const auto &device = context.device_set[id];
    assert(device != nullptr);

    // !!! This acquires a reference on the ANativeWindow !!!
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    // target surface
    ACameraOutputTarget *target{};
    ACameraOutputTarget_create(window, std::addressof(target));

    // ACaptureRequest: how to capture
    ACaptureRequest *request{};
    // capture as a preview
    // TEMPLATE_RECORD, TEMPLATE_PREVIEW, TEMPLATE_MANUAL,
    status = ACameraDevice_createCaptureRequest(device, TEMPLATE_PREVIEW,
                                                &request);
    assert(status == ACAMERA_OK);
    // designate target surface in request
    ACaptureRequest_addTarget(request, target);

    // ACaptureSessionOutput: ??

    // session output container for multiplexing
    ACaptureSessionOutputContainer *container{};
    ACaptureSessionOutputContainer_create(&container);
    // session output
    ACaptureSessionOutput *output{};
    ACaptureSessionOutput_create(window, &output);

    ACaptureSessionOutputContainer_add(container, output);

    // Hook to sync with session states
    ACameraCaptureSession_stateCallbacks stateCallbacks{};
    stateCallbacks.context = std::addressof(context);
    stateCallbacks.onReady = reinterpret_cast<ACameraCaptureSession_stateCallback>(context_on_session_ready);
    stateCallbacks.onClosed = reinterpret_cast<ACameraCaptureSession_stateCallback>(context_on_session_closed);
    stateCallbacks.onActive = reinterpret_cast<ACameraCaptureSession_stateCallback>(context_on_session_active);

    // Create a capture session
    status = ACameraDevice_createCaptureSession(
        context.device_set[id], container, &stateCallbacks,
        std::addressof(context.session_set[id]));
    assert(status == ACAMERA_OK);

    // We are going to start capture. prepare callbacks...
    ACameraCaptureSession_captureCallbacks captureCallbacks{};
    captureCallbacks.context = std::addressof(context);
    captureCallbacks.onCaptureStarted = reinterpret_cast<ACameraCaptureSession_captureCallback_start>(context_on_capture_started);
    captureCallbacks.onCaptureBufferLost = nullptr;
    captureCallbacks.onCaptureCompleted = reinterpret_cast<ACameraCaptureSession_captureCallback_result>(context_on_capture_completed);
    captureCallbacks.onCaptureFailed = reinterpret_cast<ACameraCaptureSession_captureCallback_failed>(context_on_capture_failed);
    captureCallbacks.onCaptureProgressed = nullptr;
    captureCallbacks.onCaptureSequenceAborted = nullptr;
    captureCallbacks.onCaptureSequenceCompleted = nullptr;
    // start repeating
    status = ACameraCaptureSession_setRepeatingRequest(
        context.session_set[id], &captureCallbacks, 1, &request,
        std::addressof(context.seq_id_set[id]));
    assert(status == ACAMERA_OK);

    //ACaptureSessionOutput_free(output);
    //ACaptureSessionOutputContainer_free(container);

    ANativeWindow_release(window);
}

void Java_ndcam_Device_startCapture(JNIEnv *env, jobject instance,
                                    jobject surface) noexcept
{
    if (context.manager == nullptr) // not initialized
        return;

    auto id = env->GetShortField(instance, device_id_f);
    assert(id != -1);
}

void Java_ndcam_Device_stop(JNIEnv *env, jobject instance) noexcept
{
    if (context.manager == nullptr) // not initialized
        return;

    const auto id = env->GetShortField(instance, device_id_f);
    assert(id != -1);
    // ACameraCaptureSession_setRepeatingRequest
    ACameraCaptureSession_stopRepeating(context.session_set[id]);
    context.session_set[id] = nullptr;
    context.seq_id_set[id] = 0;
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
auto error_status_message(camera_status_t status) noexcept -> const char *
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
