#include <engine/renderer/renderer_system.h>

@import <Metal/Metal.h>

// TODO: fetch without using extern
extern id metalLayer;

namespace xc::renderer {
    auto initialize() -> bool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        id<MTLCommandQueue> command_queue = [device newCommandQueue];
        //metalLayer.device = MTLCreateSystemDefaultDevice();
        //device = metalLayer.device;
        return true;
    }

    auto uninitialize() -> void {

    }

    auto tick() -> void {

    }
};