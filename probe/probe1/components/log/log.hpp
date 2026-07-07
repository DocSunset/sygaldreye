// Copyright 2025 Travis West
#pragma once

// Unified logging: Android logcat on device, the JS console in the
// browser, spdlog on host.
// Usage: LOG_E("my_tag", "format %s %d", str, val);

#ifdef __ANDROID__
#  include <android/log.h>
#  define LOG_D(tag, ...) __android_log_print(ANDROID_LOG_DEBUG, tag, __VA_ARGS__)
#  define LOG_I(tag, ...) __android_log_print(ANDROID_LOG_INFO,  tag, __VA_ARGS__)
#  define LOG_W(tag, ...) __android_log_print(ANDROID_LOG_WARN,  tag, __VA_ARGS__)
#  define LOG_E(tag, ...) __android_log_print(ANDROID_LOG_ERROR, tag, __VA_ARGS__)
#elif defined(__EMSCRIPTEN__)
#  include <cstdio>
#  define LOG_D(tag, ...) (std::printf("[%s] ", tag), std::printf(__VA_ARGS__), std::printf("\n"))
#  define LOG_I(tag, ...) (std::printf("[%s] ", tag), std::printf(__VA_ARGS__), std::printf("\n"))
#  define LOG_W(tag, ...) (std::fprintf(stderr, "[%s] ", tag), std::fprintf(stderr, __VA_ARGS__), std::fprintf(stderr, "\n"))
#  define LOG_E(tag, ...) (std::fprintf(stderr, "[%s] ", tag), std::fprintf(stderr, __VA_ARGS__), std::fprintf(stderr, "\n"))
#else
#  include <array>
#  include <cstdarg>
#  include <cstdio>
#  include <spdlog/spdlog.h>
   namespace log_detail {
   // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
   inline void host_log(int level, const char* tag, const char* fmt, ...) {
       std::array<char, 512> buf{};
       va_list ap;
       va_start(ap, fmt);
       std::vsnprintf(buf.data(), buf.size(), fmt, ap);
       va_end(ap);
       switch (level) {
           case 0: ::spdlog::debug("[{}] {}", tag, buf.data()); break;
           case 1: ::spdlog::info ("[{}] {}", tag, buf.data()); break;
           case 2: ::spdlog::warn ("[{}] {}", tag, buf.data()); break;
           default: ::spdlog::error("[{}] {}", tag, buf.data()); break;
       }
   }
   } // namespace log_detail
#  define LOG_D(tag, ...) ::log_detail::host_log(0, tag, __VA_ARGS__)
#  define LOG_I(tag, ...) ::log_detail::host_log(1, tag, __VA_ARGS__)
#  define LOG_W(tag, ...) ::log_detail::host_log(2, tag, __VA_ARGS__)
#  define LOG_E(tag, ...) ::log_detail::host_log(3, tag, __VA_ARGS__)
#endif
