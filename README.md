# SpaceCombatGame

This project was to be the start of a space combat game with a custom engine.

I've kept it here for my own reference as they are components I am reusing in other projects.

The useful bits:
* The platform layer which reimplements some of the C runtime (the engine uses size coding techniques and does not depend on the CRT).  A more advanced version of this is in another project though.
* The platform layer for MacOS also does this, including some fuctions and templates to interface with objective-c apis without using that language
* The OpenGL and Vulkan render systems have very tiny inline function loaders
* Some container classes that do not depend on external libraries
* A fast delegate implementation
* Some useful macros and a wyhash implementation