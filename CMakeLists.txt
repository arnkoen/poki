cmake_minimum_required (VERSION 3.11)
project(POKI)

add_library(poki poki.h poki.c deps.c)
target_compile_definitions(poki PUBLIC -DSOKOL_GLCORE)

add_executable(playsound playsounds.c poki.c poki.h deps.c)
target_compile_definitions(playsound PUBLIC -DPK_NO_SAPP -DSOKOL_GLCORE)

add_executable(viewgltf viewgltf.c)
target_link_libraries(viewgltf PRIVATE poki)

add_executable(viewm3d viewm3d.c)
target_link_libraries(viewm3d PRIVATE poki)

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(playsound PRIVATE GL asound dl pthread m)
    target_link_libraries(viewgltf PRIVATE X11 Xi Xcursor GL asound dl pthread m)
    target_link_libraries(viewm3d PRIVATE X11 Xi Xcursor GL asound dl pthread m)
endif()
