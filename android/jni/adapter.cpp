#include <magic/coroutine.hpp>

#include "adapter.h"

#define PROLOGUE __attribute__((constructor))
#define EPILOGUE __attribute__((destructor))

constexpr auto tag = "ndk_camera";

std::shared_ptr<spdlog::logger> logger{};
jclass illegal_argument_exception{};


// https://github.com/gabime/spdlog
PROLOGUE void jni_on_load(void) noexcept
{
    auto check_coroutine_available = [=]() -> magic::unplug
    {
        co_await magic::stdex::suspend_never{};
        // see fmt::format for usage
        logger->info("{}.{}", tag, nullptr);
    };

    // log will print thread id and message
    spdlog::set_pattern("[thread %t] %v");
    logger = spdlog::android_logger_st("android", tag);

    check_coroutine_available();
}

// Auto releasing Asset
using CameraManager = std::unique_ptr<ACameraManager, void(*)(ACameraManager*)>;


void Java_ndcam_CameraModel_Init(JNIEnv *env, jclass type) noexcept
{
    // Find exception class information (type info)
    if (illegal_argument_exception == nullptr)
        illegal_argument_exception = env->FindClass("java/lang/IllegalArgumentException");
    // !!! Since we can't throw if this info is null, call assert !!!
    assert(illegal_argument_exception != nullptr);
    assert(logger != nullptr);

    CameraManager manager = CameraManager{ACameraManager_create(), ACameraManager_delete};
    logger->debug("ACameraManager_create {}", (void*)manager.get() );

    ACameraIdList *cameras = nullptr;
    camera_status_t status = ACameraManager_getCameraIdList(manager.get(), &cameras);
    if (status != ACAMERA_OK)
        logger->error("ACameraManager_getCameraIdList camera_status {}", status );
    else
        logger->debug("ACameraManager_getCameraIdList cameras->numCameras {}", cameras->numCameras );

    std::for_each(cameras->cameraIds, cameras->cameraIds + cameras->numCameras, [&](const auto id){
        ACameraMetadata * metadata = nullptr;
        status = ACameraManager_getCameraCharacteristics(
                manager.get(), id, &metadata );

        if (status != ACAMERA_OK)
            logger->error("ACameraManager_getCameraCharacteristics camera_status {}", status );
        else
            logger->debug("ACameraManager_getCameraCharacteristics {} {}", id, (void*)metadata);

        ACameraMetadata_free(metadata);
    });
    ACameraManager_deleteCameraIdList(cameras);

    ACameraDevice* device{};
    ACameraDevice_StateCallbacks callbacks{};
    callbacks.context = nullptr;
    // callbacks.onDisconnected(nullptr, device);
    // callbacks.onError(nullptr, device, errno);
    status = ACameraManager_openCamera(manager.get(), nullptr, &callbacks, &device);

    if(status == ACAMERA_OK){
        ACameraDevice_close(device);
        device = nullptr;
    }

//    const auto request_template = ACameraDevice_request_template::TEMPLATE_PREVIEW;
//
//    // https://github.com/justinjoy/native-camera2/blob/master/app/src/main/jni/native-camera2-jni.cpp
//    ACameraCaptureSession* session{};
//    ACameraCaptureSession_stateCallbacks stateCallbacks{};
//    stateCallbacks.context = nullptr;
//    stateCallbacks.onActive(nullptr, session);
//    stateCallbacks.onClosed(nullptr, session);
//    stateCallbacks.onReady(nullptr, session);
//
//    ACameraCaptureSession_captureCallbacks captureCallbacks{};
    logger->warn("ndk_camera is under develop");
//
//    int cap_sequence = 0;
//    ACaptureRequest requests[2]{};
//    ACameraOutputTarget_create(nullptr, nullptr);
//    ACameraCaptureSession_capture(&session,  &captureCallbacks, 2, requests, &cap_sequence);
//    ACameraCaptureSession_close(session);
//    ACameraCaptureSession_abortCaptures(session);
//
//    ACameraCaptureSession_setRepeatingRequest(session, &captureCallbacks, 2, requests, &cap_sequence);
//    ACameraCaptureSession_stopRepeating(session);
//
//    ACaptureSessionOutput* output;
//    ACaptureSessionOutputContainer* cont;
//    ACaptureSessionOutputContainer_create(&cont);
//    ACaptureSessionOutputContainer_remove(cont, output);
//    ACaptureSessionOutputContainer_free(cont);


    return;
ThrowJavaException:
    env->ThrowNew(illegal_argument_exception, "error_message");
}
