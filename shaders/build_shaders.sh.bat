#/bin/bash 2>nul || goto :windows

# bash
echo hello Bash
ls

FORMAT="sokol_impl"
LANG="hlsl5:glsl430:glsl300es"

sokol-shdc -i phong.glsl -o out/phong.glsl.h -f $FORMAT --slang $LANG
sokol-shdc -i unlit.glsl -o out/unlit.glsl.h -f $FORMAT --slang $LANG

exit

:windows

set format=sokol_impl
set lang=hlsl5:glsl430:glsl300es

cmd /c "sokol-shdc -i phong.glsl -o out/phong.glsl.h -f %format% --slang %lang%"
cmd /c "sokol-shdc -i unlit.glsl -o out/unlit.glsl.h -f %format% --slang %lang%"

exit /b
