#include <ndk_camera.h>

extern std::shared_ptr<spdlog::logger> logger;

void context_t::release() noexcept
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

camera_status_t context_t::open_device(uint16_t id,
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
void context_t::close_device(uint16_t id) noexcept
{
    // close session
    auto& session = this->session_set[id];
    if(session)
    {
        logger->warn("session for device {} is alive. abort/closing...", id);

        ACameraCaptureSession_abortCaptures(session);
        ACameraCaptureSession_close(session);
        session = nullptr;
    }
    // close device
    auto &device = this->device_set[id];
    if (device)
    {
        // W/ACameraCaptureSession: Device is closed but session 0 is not notified
        logger->warn("closing device {} ...", id);

        ACameraDevice_close(device);
        device = nullptr;
    }
}

camera_status_t context_t::start_repeat(
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

void context_t::stop_repeat(uint16_t id) noexcept
{
    auto &session = this->session_set[id];
    if (session)
    {
        logger->warn("stop_repeat for session {} ", id);

        // follow `ACameraCaptureSession_setRepeatingRequest`
        ACameraCaptureSession_stopRepeating(session);

        ACameraCaptureSession_close(session);
        session = nullptr;
    }
    this->seq_id_set[id] = CAPTURE_SEQUENCE_ID_NONE;
}

camera_status_t context_t::start_capture(
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

void context_t::stop_capture(uint16_t id) noexcept
{
    auto &session = this->session_set[id];
    if (session)
    {
        logger->warn("stop_capture for session {} ", id);

        // follow `ACameraCaptureSession_capture`
        ACameraCaptureSession_abortCaptures(session);

        ACameraCaptureSession_close(session);
        session = nullptr;
    }
    this->seq_id_set[id] = 0;
}

// ACAMERA_LENS_FACING_FRONT
// ACAMERA_LENS_FACING_BACK
// ACAMERA_LENS_FACING_EXTERNAL
uint16_t context_t::get_facing(uint16_t id) noexcept
{
    // const ACameraMetadata*
    const auto *metadata = metadata_set[id];

    ACameraMetadata_const_entry lens_facing{};
    ACameraMetadata_getConstEntry(metadata,
                                  ACAMERA_LENS_FACING, &lens_facing);

    const auto *ptr = lens_facing.data.u8;
    return *ptr;
}

#ifdef _WIN32
#define PROLOGUE
#define EPILOGUE
#else
#define PROLOGUE __attribute__((constructor))
#define EPILOGUE __attribute__((destructor))
#endif

PROLOGUE void OnAttach(void *) noexcept(false)
{
    // On dll is attached...
    return;
}

EPILOGUE void OnDetach(void *) noexcept
{
    // On dll is detached...
    return;
}

#ifdef _WIN32
#include <Windows.h>

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
    try
    {
        if (fdwReason == DLL_PROCESS_ATTACH)
            ::OnAttach(hinstDLL);
        if (fdwReason == DLL_PROCESS_DETACH)
            ::OnDetach(hinstDLL);

        return TRUE;
    }
    catch (const std::exception &e)
    {
        ::perror(e.what());
        return FALSE;
    }
}

#endif
