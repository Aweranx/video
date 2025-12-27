#pragma once
#include <memory>
#include <mutex>

template <typename T>
class Singleton {
protected:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;
    static std::shared_ptr<T> instance_;
public:
    static std::shared_ptr<T> GetInstance() {
        static std::once_flag s_flag;
        std::call_once(s_flag,[&]() {
            // make_shared cant call private T
            instance_ = std::shared_ptr<T>(new T);
        });
        return instance_;
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::instance_ = nullptr;
