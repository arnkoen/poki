#/bin/bash 2>nul || goto :windows

# bash
echo hello Bash
ls

LANG="hlsl5:glsl430:glsl300es"

sokol-shdc -i shaders.glsl -o shaders.glsl.h --slang $LANG

exit

:windows

set format=sokol_impl
set lang=hlsl5:glsl430:glsl300es

cmd /c "sokol-shdc -i shaders.glsl -o shaders.glsl.h --slang %lang%"

exit /b
