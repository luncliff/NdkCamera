//
//  Author
//      luncliff@gmail.com
//
#pragma once
#ifndef _NDCAM_INCLUDE_H_
#define _NDCAM_INCLUDE_H_

#include <gsl/gsl>

#include <android/hardware_buffer.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <media/NdkImage.h>
#include <media/NdkImageReader.h>

#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCameraMetadataTags.h>
#include <camera/NdkCaptureRequest.h>

using native_window_ptr =
    std::unique_ptr<ANativeWindow, void (*)(ANativeWindow*)>;
using capture_session_output_container_ptr =
    std::unique_ptr<ACaptureSessionOutputContainer,
                    void (*)(ACaptureSessionOutputContainer*)>;
using capture_session_output_ptr =
    std::unique_ptr<ACaptureSessionOutput, void (*)(ACaptureSessionOutput*)>;
using capture_request_ptr =
    std::unique_ptr<ACaptureRequest, void (*)(ACaptureRequest*)>;

using camera_output_target_ptr =
    std::unique_ptr<ACameraOutputTarget, void (*)(ACameraOutputTarget*)>;

/**
 * Library context. Supports auto releasing and facade for features
 *
 * !!! All NDK type members must be public. There will be no encapsulation !!!
 */
struct camera_group_t final {
    // without external camera, 2 is enough(back + front).
    // But we will use more since there might be multiple(for now, 2) external
    // camera...
    static constexpr auto max_camera_count = 4;

  public:
    // Android camera manager. After the context is initialized, this must be
    // non-null if this variable is null, then the context is considered as 'not
    // initailized' and all operation *can* be ignored
    ACameraManager* manager = nullptr;
    ACameraIdList* id_list = nullptr;

    // cached metadata
    std::array<ACameraMetadata*, max_camera_count> metadata_set{};

    //
    // even though android system limits the number of maximum open camera
    // device, we will consider multiple camera are working concurrently.
    //
    // if element is nullptr, it means the device is not open.
    std::array<ACameraDevice*, max_camera_count> device_set{};

    // if there is no session, session pointer will be null
    std::array<ACameraCaptureSession*, max_camera_count> session_set{};

    // sequence number from capture session
    std::array<int, max_camera_count> seq_id_set{};

  public:
    camera_group_t() noexcept = default;
    // copy-move is disabled
    camera_group_t(const camera_group_t&) = delete;
    camera_group_t(camera_group_t&&) = delete;
    camera_group_t& operator=(const camera_group_t&) = delete;
    camera_group_t& operator=(camera_group_t&&) = delete;
    ~camera_group_t() noexcept {
        this->release(); // ensure release precedure
    }

  public:
    void release() noexcept;

  public:
    auto open_device(uint16_t id,
                     ACameraDevice_StateCallbacks& callbacks) noexcept
        -> camera_status_t;

    // Notice that this routine doesn't free metadata
    void close_device(uint16_t id) noexcept;

    auto start_repeat(
        uint16_t id, ANativeWindow* window,
        ACameraCaptureSession_stateCallbacks& on_session_changed,
        ACameraCaptureSession_captureCallbacks& on_capture_event) noexcept
        -> camera_status_t;
    void stop_repeat(uint16_t id) noexcept;

    auto start_capture(
        uint16_t id, ANativeWindow* window,
        ACameraCaptureSession_stateCallbacks& on_session_changed,
        ACameraCaptureSession_captureCallbacks& on_capture_event) noexcept
        -> camera_status_t;
    void stop_capture(uint16_t id) noexcept;

    // ACAMERA_LENS_FACING_FRONT
    // ACAMERA_LENS_FACING_BACK
    // ACAMERA_LENS_FACING_EXTERNAL
    uint16_t get_facing(uint16_t id) noexcept;
};

// device callbacks

void context_on_device_disconnected(camera_group_t& context,
                                    ACameraDevice* device) noexcept;

void context_on_device_error(camera_group_t& context, ACameraDevice* device,
                             int error) noexcept;

// session state callbacks

void context_on_session_active(camera_group_t& context,
                               ACameraCaptureSession* session) noexcept;

void context_on_session_closed(camera_group_t& context,
                               ACameraCaptureSession* session) noexcept;

void context_on_session_ready(camera_group_t& context,
                              ACameraCaptureSession* session) noexcept;

// capture callbacks

void context_on_capture_started(camera_group_t& context,
                                ACameraCaptureSession* session,
                                const ACaptureRequest* request,
                                uint64_t time_point) noexcept;

void context_on_capture_progressed(camera_group_t& context,
                                   ACameraCaptureSession* session,
                                   ACaptureRequest* request,
                                   const ACameraMetadata* result) noexcept;

void context_on_capture_completed(camera_group_t& context,
                                  ACameraCaptureSession* session,
                                  ACaptureRequest* request,
                                  const ACameraMetadata* result) noexcept;

void context_on_capture_failed(camera_group_t& context,
                               ACameraCaptureSession* session,
                               ACaptureRequest* request,
                               ACameraCaptureFailure* failure) noexcept;

void context_on_capture_buffer_lost(camera_group_t& context,
                                    ACameraCaptureSession* session,
                                    ACaptureRequest* request,
                                    ANativeWindow* window,
                                    int64_t frame_number) noexcept;

void context_on_capture_sequence_abort(camera_group_t& context,
                                       ACameraCaptureSession* session,
                                       int sequence_id) noexcept;

void context_on_capture_sequence_complete(camera_group_t& context,
                                          ACameraCaptureSession* session,
                                          int sequence_id,
                                          int64_t frame_number) noexcept;

// status - error code to string

auto camera_error_message(camera_status_t status) noexcept -> const char*;

#endif