
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


#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCameraMetadataTags.h>
#include <camera/NdkCaptureRequest.h>


_C_INTERFACE_ void JNICALL
Java_ndcam_CameraModel_Init(JNIEnv *env, jclass type) noexcept;

#endif //
