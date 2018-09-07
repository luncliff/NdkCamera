#include <magic/coroutine.hpp>

#include "adapter.h"

#define PROLOGUE __attribute__((constructor))
#define EPILOGUE __attribute__((destructor))

std::shared_ptr<spdlog::logger> logger{};

jclass illegal_argument_exception{};

void ndk_camera_init(JNIEnv *env, jclass type) noexcept
{
    // Find exception class information (type info)
    if (illegal_argument_exception == nullptr)
        illegal_argument_exception = env->FindClass("java/lang/IllegalArgumentException");
    // !!! Since we can't throw if this info is null, call assert !!!
    assert(illegal_argument_exception != nullptr);

    return;
ThrowJavaException:
    env->ThrowNew(illegal_argument_exception, "error_message");
}

auto check_coroutine_available() -> magic::unplug
{
    co_await magic::stdex::suspend_never{};
}

// https://github.com/gabime/spdlog
PROLOGUE void jni_on_load(void) noexcept
{
    check_coroutine_available();

    const auto tag = "ndk_camera";

    // log will print thread id and message
    spdlog::set_pattern("[thread %t] %v");
    logger = spdlog::android_logger_st("android", tag);
    // see fmt::format for usage
    logger->info("{}.{}", tag, nullptr);
}
