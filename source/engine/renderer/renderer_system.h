#ifndef ENGINE_RENDERER_RENDERER_SYSTEM_H
#define ENGINE_RENDERER_RENDERER_SYSTEM_H

namespace xc::renderer {
    auto initialize() -> bool;
    auto uninitialize() -> void;
    auto tick() -> void;

    auto swap() -> void;
}

#endif