
#ifndef JNI_ADAPTER_H_
#define JNI_ADAPTER_H_

#include <jni.h>

#include <experimental/filesystem>
#include <experimental/coroutine>
#include <cstring>
#include <cassert>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/android_sink.h>

#include <ndk_camera.h>

#include <android/hardware_buffer.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCameraMetadataTags.h>
#include <camera/NdkCaptureRequest.h>

_C_INTERFACE_ void JNICALL
Java_ndcam_CameraModel_Init(JNIEnv *env, jclass type) noexcept;

_C_INTERFACE_ jint JNICALL
Java_ndcam_CameraModel_GetDeviceCount(JNIEnv *env, jclass type) noexcept;

_C_INTERFACE_ void JNICALL
Java_ndcam_CameraModel_SetDeviceData(JNIEnv *env, jclass type, jobjectArray devices) noexcept;

_C_INTERFACE_ jbyte JNICALL
Java_ndcam_Device_facing(JNIEnv *env, jobject instance) noexcept;

_C_INTERFACE_ void JNICALL
Java_ndcam_Device_open(JNIEnv *env, jobject instance) noexcept;

_C_INTERFACE_ void JNICALL
Java_ndcam_Device_close(JNIEnv *env, jobject instance) noexcept;

_C_INTERFACE_ void JNICALL
Java_ndcam_Device_startRepeat(JNIEnv *env, jobject instance, jobject surface) noexcept;
_C_INTERFACE_ void JNICALL
Java_ndcam_Device_stopRepeat(JNIEnv *env, jobject instance) noexcept;

_C_INTERFACE_ void JNICALL
Java_ndcam_Device_startCapture(JNIEnv *env, jobject instance, jobject surface) noexcept;
_C_INTERFACE_ void JNICALL
Java_ndcam_Device_stopCapture(JNIEnv *env, jobject instance) noexcept;

struct context_t;

_HIDDEN_ void context_on_device_disconnected(
    context_t &context,
    ACameraDevice *device) noexcept;

_HIDDEN_ void context_on_device_error(
    context_t &context,
    ACameraDevice *device, int error) noexcept;

_HIDDEN_ void context_on_session_active(
    context_t &context,
    ACameraCaptureSession *session) noexcept;
_HIDDEN_ void context_on_session_closed(
    context_t &context,
    ACameraCaptureSession *session) noexcept;

_HIDDEN_ void context_on_session_ready(
    context_t &context,
    ACameraCaptureSession *session) noexcept;

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
    ANativeWindow *window, int64_t frameNumber) noexcept;

_HIDDEN_ void context_on_capture_sequence_abort(
    context_t &context, ACameraCaptureSession *session,
    int sequenceId) noexcept;
_HIDDEN_ void context_on_capture_sequence_complete(
    context_t &context, ACameraCaptureSession *session,
    int sequenceId, int64_t frameNumber) noexcept;

_HIDDEN_ auto camera_error_message(camera_status_t status) noexcept
    -> const char *;

#endif // JNI_ADAPTER_H_
