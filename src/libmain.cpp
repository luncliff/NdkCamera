//
//  Author
//      luncliff@gmail.com
//
#include <ndk_camera.h>
#include <ndk_camera_log.h>

using namespace std;

shared_ptr<spdlog::logger> logger{};

void camera_group_t::release() noexcept {
    // close all devices
    for (uint16_t id = 0u; id < max_camera_count; ++id)
        close_device(id);

    // release all metadata
    for (auto& meta : metadata_set)
        if (meta) {
            ACameraMetadata_free(meta);
            meta = nullptr;
        }

    // remove id list
    if (id_list)
        ACameraManager_deleteCameraIdList(id_list);
    id_list = nullptr;

    // release manager (camera service)
    if (manager)
        ACameraManager_delete(manager);
    manager = nullptr;
}

camera_status_t
camera_group_t::open_device(uint16_t id,
                            ACameraDevice_StateCallbacks& callbacks) noexcept {
    auto& device = this->device_set[id];
    auto status = ACameraManager_openCamera(         //
        this->manager, this->id_list->cameraIds[id], //
        addressof(callbacks), addressof(device));
    return status;
}

// Notice that this routine doesn't free metadata
void camera_group_t::close_device(uint16_t id) noexcept {
    // close session
    auto& session = this->session_set[id];
    if (session) {
        logger->warn("session for device {} is alive. abort/closing...", id);

        // Abort all kind of requests
        ACameraCaptureSession_abortCaptures(session);
        ACameraCaptureSession_stopRepeating(session);
        // close
        ACameraCaptureSession_close(session);
        session = nullptr;
    }
    // close device
    auto& device = this->device_set[id];
    if (device) {
        // Producing meesage like following
        // W/ACameraCaptureSession: Device is closed but session 0 is not
        // notified
        //
        // Seems like ffmpeg also has same issue, but can't sure about its
        // comment...
        //
        logger->warn("closing device {} ...", id);

        ACameraDevice_close(device);
        device = nullptr;
    }
}

camera_status_t camera_group_t::start_repeat(
    uint16_t id, ANativeWindow* window,
    ACameraCaptureSession_stateCallbacks& on_session_changed,
    ACameraCaptureSession_captureCallbacks& on_capture_event) noexcept {
    camera_status_t status = ACAMERA_OK;

    // ---- target surface for camera ----
    auto target = camera_output_target_ptr{[=]() {
                                               ACameraOutputTarget* target{};
                                               ACameraOutputTarget_create(
                                                   window, addressof(target));
                                               return target;
                                           }(),
                                           ACameraOutputTarget_free};
    assert(target.get() != nullptr);

    // ---- capture request (preview) ----
    auto request = capture_request_ptr{
        [=]() {
            ACaptureRequest* ptr{};
            // capture as a preview
            // TEMPLATE_RECORD, TEMPLATE_PREVIEW, TEMPLATE_MANUAL,
            const auto status = ACameraDevice_createCaptureRequest(
                this->device_set[id], TEMPLATE_PREVIEW, &ptr);
            assert(status == ACAMERA_OK);
            return ptr;
        }(),
        ACaptureRequest_free};
    assert(request.get() != nullptr);

    // `ACaptureRequest` == how to capture
    // detailed config comes here...
    // ACaptureRequest_setEntry_*
    // - ACAMERA_REQUEST_MAX_NUM_OUTPUT_STREAMS
    // -

    // designate target surface in request
    status = ACaptureRequest_addTarget(request.get(), target.get());
    assert(status == ACAMERA_OK);
    // ---- session output ----

    // container for multiplexing of session output
    auto container = capture_session_output_container_ptr{
        []() {
            ACaptureSessionOutputContainer* container{};
            ACaptureSessionOutputContainer_create(&container);
            return container;
        }(),
        ACaptureSessionOutputContainer_free};
    assert(container.get() != nullptr);

    // session output
    auto output = capture_session_output_ptr{
        [=]() {
            ACaptureSessionOutput* output{};
            ACaptureSessionOutput_create(window, &output);
            return output;
        }(),
        ACaptureSessionOutput_free};
    assert(output.get() != nullptr);

    status = ACaptureSessionOutputContainer_add(container.get(), output.get());
    assert(status == ACAMERA_OK);

    // ---- create a session ----
    status = ACameraDevice_createCaptureSession(
        this->device_set[id], container.get(), addressof(on_session_changed),
        addressof(this->session_set[id]));
    assert(status == ACAMERA_OK);

    // ---- set request ----
    array<ACaptureRequest*, 1> batch_request{};
    batch_request[0] = request.get();

    status = ACameraCaptureSession_setRepeatingRequest(
        this->session_set[id], addressof(on_capture_event),
        batch_request.size(), batch_request.data(),
        addressof(this->seq_id_set[id]));
    assert(status == ACAMERA_OK);

    status =
        ACaptureSessionOutputContainer_remove(container.get(), output.get());
    assert(status == ACAMERA_OK);
    status = ACaptureRequest_removeTarget(request.get(), target.get());
    assert(status == ACAMERA_OK);

    return status;
}

void camera_group_t::stop_repeat(uint16_t id) noexcept {
    auto& session = this->session_set[id];
    if (session) {
        logger->warn("stop_repeat for session {} ", id);

        // follow `ACameraCaptureSession_setRepeatingRequest`
        ACameraCaptureSession_stopRepeating(session);

        ACameraCaptureSession_close(session);
        session = nullptr;
    }
    this->seq_id_set[id] = CAPTURE_SEQUENCE_ID_NONE;
}

camera_status_t camera_group_t::start_capture(
    uint16_t id, ANativeWindow* window,
    ACameraCaptureSession_stateCallbacks& on_session_changed,
    ACameraCaptureSession_captureCallbacks& on_capture_event) noexcept {
    camera_status_t status = ACAMERA_OK;

    // ---- target surface for camera ----
    auto target = camera_output_target_ptr{[=]() {
                                               ACameraOutputTarget* target{};
                                               ACameraOutputTarget_create(
                                                   window, addressof(target));
                                               return target;
                                           }(),
                                           ACameraOutputTarget_free};
    assert(target.get() != nullptr);

    // ---- capture request (preview) ----
    auto request =
        capture_request_ptr{[](ACameraDevice* device) {
                                ACaptureRequest* ptr{};
                                // capture as a preview
                                // TEMPLATE_RECORD, TEMPLATE_PREVIEW,
                                // TEMPLATE_MANUAL,
                                const auto status =
                                    ACameraDevice_createCaptureRequest(
                                        device, TEMPLATE_STILL_CAPTURE, &ptr);
                                assert(status == ACAMERA_OK);
                                return ptr;
                            }(this->device_set[id]),
                            ACaptureRequest_free};
    assert(request.get() != nullptr);

    // `ACaptureRequest` == how to capture
    // detailed config comes here...
    // ACaptureRequest_setEntry_*
    // - ACAMERA_REQUEST_MAX_NUM_OUTPUT_STREAMS
    // -

    // designate target surface in request
    status = ACaptureRequest_addTarget(request.get(), target.get());
    assert(status == ACAMERA_OK);
    // defer    ACaptureRequest_removeTarget;

    // ---- session output ----

    // container for multiplexing of session output
    auto container = capture_session_output_container_ptr{
        []() {
            ACaptureSessionOutputContainer* container{};
            ACaptureSessionOutputContainer_create(&container);
            return container;
        }(),
        ACaptureSessionOutputContainer_free};
    assert(container.get() != nullptr);

    // session output
    auto output = capture_session_output_ptr{
        [=]() {
            ACaptureSessionOutput* output{};
            ACaptureSessionOutput_create(window, &output);
            return output;
        }(),
        ACaptureSessionOutput_free};
    assert(output.get() != nullptr);

    status = ACaptureSessionOutputContainer_add(container.get(), output.get());
    assert(status == ACAMERA_OK);
    // defer ACaptureSessionOutputContainer_remove

    // ---- create a session ----
    status = ACameraDevice_createCaptureSession(
        this->device_set[id], container.get(), addressof(on_session_changed),
        addressof(this->session_set[id]));
    assert(status == ACAMERA_OK);

    // ---- set request ----
    array<ACaptureRequest*, 1> batch_request{};
    batch_request[0] = request.get();

    status = ACameraCaptureSession_capture(
        this->session_set[id], addressof(on_capture_event),
        batch_request.size(), batch_request.data(),
        addressof(this->seq_id_set[id]));
    assert(status == ACAMERA_OK);

    status =
        ACaptureSessionOutputContainer_remove(container.get(), output.get());
    assert(status == ACAMERA_OK);

    status = ACaptureRequest_removeTarget(request.get(), target.get());
    assert(status == ACAMERA_OK);

    return status;
}

void camera_group_t::stop_capture(uint16_t id) noexcept {
    auto& session = this->session_set[id];
    if (session) {
        logger->warn("stop_capture for session {} ", id);

        // follow `ACameraCaptureSession_capture`
        ACameraCaptureSession_abortCaptures(session);

        ACameraCaptureSession_close(session);
        session = nullptr;
    }
    this->seq_id_set[id] = 0;
}

auto camera_group_t::get_facing(uint16_t id) noexcept -> uint16_t {
    // const ACameraMetadata*
    const auto* metadata = metadata_set[id];

    ACameraMetadata_const_entry entry{};
    ACameraMetadata_getConstEntry(metadata, ACAMERA_LENS_FACING, &entry);

    // lens facing
    const auto facing = *(entry.data.u8);
    assert(
        facing == ACAMERA_LENS_FACING_FRONT || // ACAMERA_LENS_FACING_FRONT
        facing == ACAMERA_LENS_FACING_BACK ||  // ACAMERA_LENS_FACING_BACK
        facing == ACAMERA_LENS_FACING_EXTERNAL // ACAMERA_LENS_FACING_EXTERNAL
    );
    return facing;
}

__attribute__((constructor)) void on_ndkcamera_attach() noexcept(false) {
    return;
}

__attribute__((destructor)) void on_ndkcamera_detach() noexcept {
    return;
}
