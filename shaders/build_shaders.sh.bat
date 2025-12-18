#/bin/bash 2>nul || goto :windows

# bash
echo hello Bash
ls

LANG="hlsl5:glsl430:wgsl"

sokol-shdc -i shaders.glsl -o shaders.glsl.h --slang $LANG
sokol-shdc -i gen_mips.glsl -o gen_mips.glsl.h --slang $LANG

exit

:windows

set format=sokol_impl
set lang=hlsl5:glsl430:wgsl

cmd /c "sokol-shdc -i shaders.glsl -o shaders.glsl.h --slang %lang%"
cmd /c "sokol-shdc -i gen_mips.glsl -o gen_mips.glsl.h --slang %lang%"

exit /b
