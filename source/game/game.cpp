// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <engine/platform/platform_system.h>
//#include <engine/renderer/renderer_system.h>

//extern bool running;

extern "C" auto entry() -> void {
    //if (!xc::platform::initialize()) xc::platform::exit(-1);
    //if (!xc::renderer::initialize()) xc::platform::exit(-1);

    //while (running) {
    //    xc::platform::tick();
    //}

    //xc::platform::uninitialize();

    xc::platform::exit(0);
}


// TODO:
// small buffer optimization for array
// event system
// replace extern bool running with events?
