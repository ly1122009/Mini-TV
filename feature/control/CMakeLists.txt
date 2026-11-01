add_library(ncl_control STATIC src/ncl_control.c)
target_include_directories(ncl_control PUBLIC inc)
target_link_libraries(ncl_control PUBLIC ncl_interface ncl_player PkgConfig::GST)
