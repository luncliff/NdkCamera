
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
// #include <android/native_activity.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCameraMetadataTags.h>
#include <camera/NdkCaptureRequest.h>

//struct window_lock
//{
//    ANativeWindow* window;
//
//    void lock() noexcept
//    {
//        ANativeWindow_acquire(window);
//    }
//    void unlock() noexcept
//    {
//        ANativeWindow_release(window);
//    }
//};

_C_INTERFACE_ void JNICALL
Java_ndcam_CameraModel_Init(JNIEnv *env, jclass type) noexcept;

_C_INTERFACE_ jint JNICALL
Java_ndcam_CameraModel_GetDeviceCount(JNIEnv *env, jclass type) noexcept;

_C_INTERFACE_ void JNICALL
Java_ndcam_CameraModel_SetDeviceData(JNIEnv *env, jclass type, jobjectArray devices) noexcept;

_C_INTERFACE_ jboolean JNICALL
Java_ndcam_Device_isFront(JNIEnv *env, jobject instance) noexcept;

_C_INTERFACE_ jboolean JNICALL
Java_ndcam_Device_isBack(JNIEnv *env, jobject instance) noexcept;

_C_INTERFACE_ jboolean JNICALL
Java_ndcam_Device_isExternal(JNIEnv *env, jobject instance) noexcept;

_C_INTERFACE_ void JNICALL
Java_ndcam_Device_startRepeat(JNIEnv *env, jobject instance, jobject surface) noexcept;
_C_INTERFACE_ void JNICALL
Java_ndcam_Device_startCapture(JNIEnv *env, jobject instance, jobject surface) noexcept;

_C_INTERFACE_ void JNICALL
Java_ndcam_Device_stop(JNIEnv *env, jobject instance) noexcept;

_HIDDEN_ auto error_status_message(camera_status_t status) noexcept
    -> const char *;

#endif // JNI_ADAPTER_H_
