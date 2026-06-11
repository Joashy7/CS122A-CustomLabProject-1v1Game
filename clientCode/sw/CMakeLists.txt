set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "Build LVGL examples" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "Build LVGL demos" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "Use internal ThorVG" FORCE)
set(LV_BUILD_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/lv_conf.h CACHE PATH "LVGL config" FORCE)
add_subdirectory(lvgl)

include_directories(include)
include_directories(lvgl)

add_custom_target(sw ALL
     DEPENDS
          cs122_demo
)

add_definitions(-DBUILD_LVGL=1)
add_executable(cs122_demo
     src/main.cpp
     src/spi_display.cpp
     src/cs122_app.cpp
     src/lvgl_example_grid.cpp
)

pico_enable_stdio_usb(cs122_demo 1)

# pull in common dependencies and additional spi hardware support
target_link_libraries(cs122_demo pico_stdlib hardware_spi hardware_adc pico_cyw43_arch_lwip_poll lvgl::lvgl)

# create map/bin/hex file etc.
pico_add_extra_outputs(cs122_demo)

