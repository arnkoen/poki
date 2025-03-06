cmake_minimum_required(VERSION 3.21)
project(POKI)

# include the fips main cmake file
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

fips_setup()
add_definitions(-DSOKOL_NO_DEPRECATED)
add_definitions(-DSOKOL_GLCORE)
set(slang "hlsl5:glsl430:glsl300es")

fips_begin_lib(poki)
  fips_src(code GROUP code)
  fips_src(deps GROUP deps)
  fips_src(shaders/out GROUP shaders)
  target_include_directories(poki PUBLIC code)
  target_include_directories(poki PUBLIC deps)
  target_include_directories(poki PUBLIC shaders/out)
  if(FIPS_LINUX)
    fips_deps(X11 Xi Xcursor GL asound dl pthread m)
  endif()
fips_end_lib()

fips_ide_group(apps)
add_subdirectory(apps)
