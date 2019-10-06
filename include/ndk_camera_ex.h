//
//  Author
//      luncliff@gmail.com
//
#pragma once
#ifndef _NDCAM_INCLUDE_EX_H_
#define _NDCAM_INCLUDE_EX_H_

#include <ndk_camera.h>

#if __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#include <new>

struct promise_capture_session_t {
    auto inital_suspend() noexcept {
        return std::experimental::suspend_always{};
    }
    auto inital_suspend() noexcept {
        return std::experimental::suspend_always{};
    }
    void unhandled_exception() noexcept(false) {
        throw;
    }
};

class capture_session_t {
  public:
    class promise_type : public promise_capture_session_t {
        void return_void() noexcept {
            // ...
        };
        auto get_return_object() noexcept -> promise_type* {
            return this;
        }
    };

  public:
    capture_session_t(promise_type* p) noexcept {
    }
};

#endif // <experimental/coroutine>
#endif // _NDCAM_INCLUDE_EX_H_
