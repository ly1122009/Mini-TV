add_library(ncl_config STATIC src/ncl_config.c)
target_include_directories(ncl_config PUBLIC inc)
target_link_libraries(ncl_config PUBLIC ncl_interface PkgConfig::GST)
