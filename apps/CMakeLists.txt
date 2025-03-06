fips_begin_app(hello windowed)
  fipsutil_copy(assets.yml)
  fips_files(hello.c)
  fips_deps(poki)
fips_end_app()

