# poki
Poki (pronounced pokee) is a small WIP framework for games and creative coding. \
Features:
- model loading (gltf/glb and m3d)
- gltf keyframe animations
- m3d skeletal animations
- qoi, png, webp and dds image/texture loading
- ogg sound loading and spatial playback
- phong shading

Where is the ui-stuff? And where are the functions for 2D drawing? What about noise?
There are other libraries, which already do these things really well and are really easy to integrate with poki.
Have a look at [sokol_imgui](https://github.com/floooh/sokol/blob/master/util/sokol_imgui.h),
[sokol_gp](https://github.com/edubart/sokol_gp/tree/master) and [FastNoiseLite](https://github.com/Auburn/FastNoiseLite/tree/master/C).
For text rendering and generating 3D-geometry you can have a look [here](https://github.com/floooh/sokol/tree/master/util). \
If you still feel, that there are essential things missing, feel free to open an issue.

Audio has been moved outside the core. This is because it does not support sound input, which is not
optimal for some creative coding use cases. Have a look at libs like miniaudio or rtAudio, if you need it.
Also, if you'd like some advanced audio engine for games, you should have a look at [SoLoud](https://solhsa.com/soloud/).

The built-in phong shader uses the alpha channel of the diffuse texture as a specular map.
If you want to pack your image accordingly, you can use [pk-image](https://github.com/arnkov/pk-image). \
For tools regarding the m3d model format, have a look here https://gitlab.com/bztsrc/model3d.

The license for poki itself is the [uLicense](https://github.com/r-lyeh/uLicense). For the licenses
of the dependencies, please have a look at the headers in the "deps" folder.

Thanks:
- [RandyGaul](https://github.com/RandyGaul) - [cute_png.h](https://github.com/RandyGaul/cute_headers/blob/master/cute_png.h)
- [HandmadeMath](https://github.com/HandmadeMath) - For their amazing single header math library
- [bzt](https://gitlab.com/bztsrc) - for the [m3d](https://gitlab.com/bztsrc/model3d) model format
- [Johannes Kuhlmann](https://github.com/jkuhlmann) - for [cgltf](https://github.com/jkuhlmann/cgltf)
- [Dominic Szablewski](https://github.com/phoboslab) - for the [“Quite OK Image Format”](https://github.com/phoboslab/qoi)
- [Matthew Endsley](https://github.com/mendsley) - for the original c++ version of tinymixer.
- [Sean Barrett](https://github.com/nothings) - for [stb_vorbis](https://github.com/nothings/stb)
- [Andre Weissflog](https://github.com/floooh) - for the [sokol](https://github.com/floooh/sokol) headers
- [r-lyeh](https://github.com/r-lyeh) - [uLicense](https://github.com/r-lyeh/uLicense) and the dual .sh.bat hack
- [Krzysztof Gabis](https://github.com/kgabis) - join.py
- [justus2510](https://github.com/justus2510) - for [tiny-webp](https://github.com/justus2510/tiny-webp)
- [Sepehr Taghdisian](https://github.com/septag) - for [dds-ktx](https://github.com/septag/dds-ktx)
- [Cody Olivier](https://github.com/thebeast33) - for [cro_mipmap](https://github.com/thebeast33/cro_lib)

# building
There is a small CMake script as an example, but building poki is very easy, you just need to compile poki.c.
It's recommended to also define the sokol graphics backend in your build script (SOKOL_GLCORE, SOKOL_D3D11, etc...).
If you don't want to use sokol_app.h, you can define PK_NO_SAPP in your build script.
The Cmake script should work on Linux and Windows, but the project itself *should* build on macOS and for the browser via emscripten too. \
I do not have the time and ressources to test compilation on mac and clang on a regular basis. Feel free to open an issue, if you run into problems. \
For rebuilding the shaders, you'll need [sokol-shdc](https://github.com/floooh/sokol-tools-bin) somewhere in your path.
For more info have a look at the sokol-shdc docs and at the script inside the "shaders" directory.



