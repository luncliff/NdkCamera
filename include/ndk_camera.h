// ---------------------------------------------------------------------------
//
//  Author
//      luncliff@gmail.com
//
// ---------------------------------------------------------------------------
#ifndef _LINKABLE_DLL_MACRO_
#define _LINKABLE_DLL_MACRO_

#ifdef _WIN32
#define _HIDDEN_
#ifdef _WINDLL
#define _INTERFACE_ __declspec(dllexport)
#define _C_INTERFACE_ extern "C" __declspec(dllexport)
#else
#define _INTERFACE_ __declspec(dllimport)
#define _C_INTERFACE_ extern "C" __declspec(dllimport)
#endif

#elif __APPLE__ || __linux__ || __unix__
#define _INTERFACE_ __attribute__((visibility("default")))
#define _C_INTERFACE_ extern "C" __attribute__((visibility("default")))
#define _HIDDEN_ __attribute__((visibility("hidden")))

#else
#error "Unknown platform"
#endif

#endif // _LINKABLE_DLL_MACRO_

#ifndef _NDCAM_INCLUDE_H_
#define _NDCAM_INCLUDE_H_

#include <cassert>
#include <memory>
#include <array>

#include <jni.h>

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

#include <spdlog/spdlog.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/android_sink.h>

static constexpr auto tag_ndk_camera = "ndk_camera";

using NativeWindow =
    std::unique_ptr<ANativeWindow,
                    void (*)(ANativeWindow *)>;
using CaptureSessionOutputContainer =
    std::unique_ptr<ACaptureSessionOutputContainer,
                    void (*)(ACaptureSessionOutputContainer *)>;
using CaptureSessionOutput =
    std::unique_ptr<ACaptureSessionOutput,
                    void (*)(ACaptureSessionOutput *)>;
using CaptureRequest =
    std::unique_ptr<ACaptureRequest,
                    void (*)(ACaptureRequest *)>;

using CameraOutputTarget =
    std::unique_ptr<ACameraOutputTarget,
                    void (*)(ACameraOutputTarget *)>;

/**
 * Library context. Supports auto releasing and facade for features
 * 
 * !!! All NDK type members must be public. There will be no encapsulation !!!
 */
struct context_t final
{
    // without external camera, 2 is enough(back + front).
    // But we will use more since there might be multiple(for now, 2) external camera...
    static constexpr auto max_camera_count = 4;

  public:
    // Android camera manager. After the context is initialized, this must be non-null
    // if this variable is null, then the context is considered as
    // 'not initailized' and all operation *can* be ignored
    ACameraManager *manager = nullptr;
    ACameraIdList *id_list = nullptr;

    // cached metadata
    std::array<ACameraMetadata *, max_camera_count> metadata_set{};

    //
    // even though android system limits the number of maximum open camera device,
    // we will consider multiple camera are working concurrently.
    //
    // if element is nullptr, it means the device is not open.
    std::array<ACameraDevice *, max_camera_count> device_set{};

    // if there is no session, session pointer will be null
    std::array<ACameraCaptureSession *, max_camera_count> session_set{};

    // sequence number from capture session
    std::array<int, max_camera_count> seq_id_set{};

  public:
    context_t() noexcept = default;
    // copy-move is disabled
    context_t(const context_t &) = delete;
    context_t(context_t &&) = delete;
    context_t &operator=(const context_t &) = delete;
    context_t &operator=(context_t &&) = delete;
    ~context_t() noexcept
    {
        this->release(); // ensure release precedure
    }

  public:
    void release() noexcept;

  public:
    auto open_device(uint16_t id,
                     ACameraDevice_StateCallbacks &callbacks) noexcept
        -> camera_status_t;

    // Notice that this routine doesn't free metadata
    void close_device(uint16_t id) noexcept;

    auto start_repeat(
        uint16_t id,
        ANativeWindow *window,
        ACameraCaptureSession_stateCallbacks &on_session_changed,
        ACameraCaptureSession_captureCallbacks &on_capture_event) noexcept
        -> camera_status_t;
    void stop_repeat(uint16_t id) noexcept;

    auto start_capture(
        uint16_t id,
        ANativeWindow *window,
        ACameraCaptureSession_stateCallbacks &on_session_changed,
        ACameraCaptureSession_captureCallbacks &on_capture_event) noexcept
        -> camera_status_t;
    void stop_capture(uint16_t id) noexcept;

    // ACAMERA_LENS_FACING_FRONT
    // ACAMERA_LENS_FACING_BACK
    // ACAMERA_LENS_FACING_EXTERNAL
    uint16_t get_facing(uint16_t id) noexcept;
};

// device callbacks

_HIDDEN_ void context_on_device_disconnected(
    context_t &context,
    ACameraDevice *device) noexcept;

_HIDDEN_ void context_on_device_error(
    context_t &context,
    ACameraDevice *device, int error) noexcept;

// session state callbacks

_HIDDEN_ void context_on_session_active(
    context_t &context,
    ACameraCaptureSession *session) noexcept;

_HIDDEN_ void context_on_session_closed(
    context_t &context,
    ACameraCaptureSession *session) noexcept;

_HIDDEN_ void context_on_session_ready(
    context_t &context,
    ACameraCaptureSession *session) noexcept;

// capture callbacks

_HIDDEN_ void context_on_capture_started(
    context_t &context, ACameraCaptureSession *session,
    const ACaptureRequest *request,
    uint64_t time_point) noexcept;

_HIDDEN_ void context_on_capture_progressed(
    context_t &context, ACameraCaptureSession *session,
    ACaptureRequest *request,
    const ACameraMetadata *result) noexcept;

_HIDDEN_ void context_on_capture_completed(
    context_t &context, ACameraCaptureSession *session,
    ACaptureRequest *request,
    const ACameraMetadata *result) noexcept;

_HIDDEN_ void context_on_capture_failed(
    context_t &context, ACameraCaptureSession *session,
    ACaptureRequest *request,
    ACameraCaptureFailure *failure) noexcept;

_HIDDEN_ void context_on_capture_buffer_lost(
    context_t &context, ACameraCaptureSession *session,
    ACaptureRequest *request,
    ANativeWindow *window, int64_t frame_number) noexcept;

_HIDDEN_ void context_on_capture_sequence_abort(
    context_t &context, ACameraCaptureSession *session,
    int sequence_id) noexcept;

_HIDDEN_ void context_on_capture_sequence_complete(
    context_t &context, ACameraCaptureSession *session,
    int sequence_id, int64_t frame_number) noexcept;

// status - error string map

_HIDDEN_ auto camera_error_message(camera_status_t status) noexcept
    -> const char *;

#endif