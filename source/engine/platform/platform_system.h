#ifndef ENGINE_PLATFORM_PLATFORM_SYSTEM_H
#define ENGINE_PLATFORM_PLATFORM_SYSTEM_H

#include <engine/core/types.h>
#include <engine/core/string.h>

namespace xc::platform {
    auto initialize() -> bool;
    auto uninitialize() -> void;
    auto tick() -> void;

    auto exit(int code) -> void;

    auto load_library(char const* name) -> void*;
    auto unload_library(void* library) -> void;

    auto load_function(void* library, char const* name) -> void*;
}

auto print(const char *format, ...) -> void;

#endif