# poki
Poki (pronounced pokee) is a small creative coding framework.
Features:
- model loading (gltf/glb and m3d)
- gltf keyframe animations
- m3d skeletal animations
- qoi and png image loading
- ogg sound loading and spatial playback
- phong shading

The builtin phong shader uses the alpha channel of the diffuse texture as a specular map.
If you want to pack your image accordingly, you can use [pk-image](https://github.com/arnkov/pk-image). \
For tools regarding the m3d model format, have a look here https://gitlab.com/bztsrc/model3d. \
The license for poki itself is the [uLicense](https://github.com/r-lyeh/uLicense). For the licenses
of the dependencies, please have a look at the headers in the "deps" folder.

# building
There is a small build script called build.sh.bat. On Windows just type "build.sh.bat", on Linux use "sh ./build.sh.bat".
You can also use the Cmake script, of course you know, how to do it. It's recommended, to also define the sokol graphics
backend in your build script (SOKOL_GLCORE, SOKOL_D3D11, etc...). If you don't want to use sokol_app.h, you can define
PK_NO_SAPP in your build script. Of course you can also just drop the "deps" folder, poki.h, poki.c and deps.c into your
project and build it however you want.
