//
//  Author
//      luncliff@gmail.com
//
#ifndef NDK_CAMERA_JNI_ADAPTER_H
#define NDK_CAMERA_JNI_ADAPTER_H

#define _INTERFACE_ __attribute__((visibility("default")))
#define _C_INTERFACE_ extern "C" __attribute__((visibility("default")))
#define _HIDDEN_ __attribute__((visibility("hidden")))

#include <jni.h>

_C_INTERFACE_ void JNICALL //
Java_ndcam_CameraModel_Init(JNIEnv* env, jclass type) noexcept;

_C_INTERFACE_ jint JNICALL //
Java_ndcam_CameraModel_GetDeviceCount(JNIEnv* env, jclass type) noexcept;

_C_INTERFACE_ void JNICALL //
Java_ndcam_CameraModel_SetDeviceData(JNIEnv* env, jclass type,
                                     jobjectArray devices) noexcept;

_C_INTERFACE_ jbyte JNICALL //
Java_ndcam_Device_facing(JNIEnv* env, jobject instance) noexcept;

_C_INTERFACE_ void JNICALL //
Java_ndcam_Device_open(JNIEnv* env, jobject instance) noexcept;

_C_INTERFACE_ void JNICALL //
Java_ndcam_Device_close(JNIEnv* env, jobject instance) noexcept;

_C_INTERFACE_ void JNICALL //
Java_ndcam_Device_startRepeat(JNIEnv* env, jobject instance,
                              jobject surface) noexcept;
_C_INTERFACE_ void JNICALL //
Java_ndcam_Device_stopRepeat(JNIEnv* env, jobject instance) noexcept;

_C_INTERFACE_ void JNICALL //
Java_ndcam_Device_startCapture(JNIEnv* env, jobject instance,
                               jobject surface) noexcept;
_C_INTERFACE_ void JNICALL //
Java_ndcam_Device_stopCapture(JNIEnv* env, jobject instance) noexcept;

#endif // JNI_ADAPTER_H
