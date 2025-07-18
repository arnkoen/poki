#Just an example for compiling poki and the examples.
cmake_minimum_required (VERSION 3.11)
project(POKI)

add_library(poki poki.h poki.c pk_deps.c)
target_compile_definitions(poki PUBLIC -DSOKOL_GLCORE)

add_library(pkaudio
    pk_audio/pk_audio_deps.c
    pk_audio/pk_audio.c
    pk_audio/pk_audio.h
)


add_executable(playsound examples/playsound.c)
target_link_libraries(playsound PRIVATE poki pkaudio)

add_executable(viewgltf examples/viewgltf.c)
target_link_libraries(viewgltf PRIVATE poki)

add_executable(viewm3d examples/viewm3d.c)
target_link_libraries(viewm3d PRIVATE poki)

add_executable(single_header examples/single_header.c)

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(playsound PRIVATE X11 Xi Xcursor GL asound dl pthread m)
    target_link_libraries(viewgltf PRIVATE X11 Xi Xcursor GL asound dl pthread m)
    target_link_libraries(viewm3d PRIVATE X11 Xi Xcursor GL asound dl pthread m)
    target_link_libraries(single_header PRIVATE X11 Xi Xcursor GL asound dl pthread m)
endif()
