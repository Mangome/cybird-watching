cmake_minimum_required(VERSION 3.15)
project(lvgl_simulator C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# LVGL库路径
set(LVGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/lvgl)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LVGL_DIR}
)

# 收集LVGL源文件（排除示例）
file(GLOB_RECURSE LVGL_SOURCES
    ${LVGL_DIR}/src/lv_core/*.c
    ${LVGL_DIR}/src/lv_draw/*.c
    ${LVGL_DIR}/src/lv_font/*.c
    ${LVGL_DIR}/src/lv_hal/*.c
    ${LVGL_DIR}/src/lv_misc/*.c
    ${LVGL_DIR}/src/lv_themes/*.c
    ${LVGL_DIR}/src/lv_widgets/*.c
)

# 项目源文件
set(PROJECT_SOURCES
    src/main.c
    src/hal/sdl_display.c
    src/hal/sdl_mouse.c
    src/hal/file_loader.c
)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${LVGL_SOURCES}
)

# 查找SDL3
find_package(SDL3 REQUIRED)
target_link_libraries(${PROJECT_NAME} SDL3::SDL3)

# 编译定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    LV_CONF_PATH=${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h
)

# 复制资源文件到构建目录
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Windows平台自动复制SDL3.dll
if(WIN32)
    # 查找SDL3.dll
    find_file(SDL3_DLL
        NAMES SDL3.dll
        PATHS
            ${CMAKE_PREFIX_PATH}/bin
            $ENV{SDL3_DIR}/bin
        NO_DEFAULT_PATH
    )
    
    if(SDL3_DLL)
        message(STATUS "Found SDL3.dll: ${SDL3_DLL}")
        # 复制到构建目录
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL3_DLL}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying SDL3.dll to output directory"
        )
    else()
        message(WARNING "SDL3.dll not found, you may need to copy it manually")
    endif()
endif()

message(STATUS "====================================")
message(STATUS "LVGL Simulator Configuration")
message(STATUS "====================================")
message(STATUS "LVGL Directory: ${LVGL_DIR}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "====================================")
