# poki
Poki (pronounced pokee) is a small creative coding framework. It's basically just some glue between libraries other people wrote.
Right now it is only tested on windows, linux will follow soon. This is a work in progress, so anything here might be buggy or subject to changes. \
Features:
- model loading (gltf/glb and m3d)
- .gltf keyframe animations (skeletal not yet supported)
- .qoi image loading
- .ogg sound loading and spatial playback
- phong shading

The builtin phong shader uses the alpha channel of the diffuse texture as a specular map.
If you want to pack your image accordingly, you can use [pk-image](https://github.com/arnkov/pk-image).

# building
This project uses the [fips](https://github.com/floooh/fips) build system.
Although poki's public api is c compatible, you will need a c++ compiler. Also you will need python and cmake installed.

- md MY_WORKSPACE && cd MY_WORKSPACE
- git clone https://github.com/arnkov/poki.git
- cd poki
- fips build
- fips run hello

If you run into issues, you can run "fips diag" to get some information about what might be going wrong. Sometimes also running "fips clean", "fips gen", "fips build" does the job.

