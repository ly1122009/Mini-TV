add_library(ncl_player STATIC src/ncl_player.c)
target_include_directories(ncl_player PUBLIC inc)
target_link_libraries(ncl_player PUBLIC ncl_interface ncl_config PkgConfig::GST)
