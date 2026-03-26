# Phoenix Engine Platform Layer CMakeLists.txt
# 跨平台支持构建配置

cmake_minimum_required(VERSION 3.20)

# ============================================================================
# 平台检测
# ============================================================================

if(WIN32)
    set(PHOENIX_PLATFORM_NAME "Windows")
    set(PHOENIX_PLATFORM_SOURCES
        windows/win32_window.cpp
        windows/dx12_device.cpp
        windows/dxgi_manager.cpp
    )
    set(PHOENIX_PLATFORM_LIBS
        d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib
        dwmapi.lib psapi.lib
    )
elseif(APPLE)
    if(IOS)
        set(PHOENIX_PLATFORM_NAME "iOS")
        set(PHOENIX_PLATFORM_SOURCES
            ios/ios_window.mm
            ios/metal_device_ios.mm
            ios/ios_input.mm
        )
        set(PHOENIX_PLATFORM_FRAMEWORKS
            Metal MetalKit UIKit QuartzCore CoreGraphics
        )
    else()
        set(PHOENIX_PLATFORM_NAME "macOS")
        set(PHOENIX_PLATFORM_SOURCES
            macos/macos_window.mm
            macos/metal_device.mm
            macos/macos_input.mm
        )
        set(PHOENIX_PLATFORM_FRAMEWORKS
            Metal MetalKit Cocoa QuartzCore CoreGraphics
        )
    endif()
elseif(ANDROID)
    set(PHOENIX_PLATFORM_NAME "Android")
    set(PHOENIX_PLATFORM_SOURCES
        android/android_platform.cpp
        android/android_window.cpp
        android/android_input.cpp
        android/vulkan_device_android.cpp
    )
    set(PHOENIX_PLATFORM_LIBS
        android log EGL GLESv3
    )
elseif(UNIX)
    set(PHOENIX_PLATFORM_NAME "Linux")
    set(PHOENIX_PLATFORM_SOURCES
        linux/linux_window.cpp
        linux/vulkan_device.cpp
        linux/linux_input.cpp
    )
    set(PHOENIX_PLATFORM_LIBS
        dl pthread
    )
    # 查找 X11/Wayland
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(X11 REQUIRED x11)
    pkg_check_modules(WAYLAND wayland-client)
    list(APPEND PHOENIX_PLATFORM_LIBS ${X11_LIBRARIES})
elseif(EMSCRIPTEN)
    set(PHOENIX_PLATFORM_NAME "Web")
    set(PHOENIX_PLATFORM_SOURCES
        webgpu/webgpu_device.cpp
        webgpu/webgl_fallback.cpp
        web/window_web.cpp
        web/input_web.cpp
    )
    set(PHOENIX_PLATFORM_LIBS)
else()
    set(PHOENIX_PLATFORM_NAME "Unknown")
    set(PHOENIX_PLATFORM_SOURCES)
    set(PHOENIX_PLATFORM_LIBS)
endif()

# ============================================================================
# 公共平台源文件
# ============================================================================

set(PHOENIX_PLATFORM_COMMON_SOURCES
    common/platform_common.cpp
    common/window_common.cpp
    common/input_common.cpp
    common/timer_common.cpp
    common/application.cpp
)

# ============================================================================
# 平台库
# ============================================================================

add_library(phoenix_platform STATIC
    ${PHOENIX_PLATFORM_SOURCES}
    ${PHOENIX_PLATFORM_COMMON_SOURCES}
)

target_include_directories(phoenix_platform PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 平台特定编译定义
target_compile_definitions(phoenix_platform PUBLIC
    PHOENIX_PLATFORM_${CMAKE_SYSTEM_NAME}=1
)

# 链接平台库
if(WIN32 OR ANDROID OR UNIX)
    target_link_libraries(phoenix_platform PRIVATE ${PHOENIX_PLATFORM_LIBS})
endif()

# macOS/iOS Frameworks
if(APPLE)
    target_link_libraries(phoenix_platform PRIVATE ${PHOENIX_PLATFORM_FRAMEWORKS})
endif()

# Emscripten 特定设置
if(EMSCRIPTEN)
    set_target_properties(phoenix_platform PROPERTIES
        COMPILE_FLAGS "-s USE_WEBGPU=1 -s MAX_WEBGL_VERSION=2"
        LINK_FLAGS "-s USE_WEBGPU=1 -s MAX_WEBGL_VERSION=2"
    )
endif()

# ============================================================================
# 编译选项
# ============================================================================

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(phoenix_platform PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(phoenix_platform PRIVATE -O3)
    endif()
elseif(MSVC)
    target_compile_options(phoenix_platform PRIVATE
        /W4 /permissive-
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(phoenix_platform PRIVATE /O2)
    endif()
endif()

# ============================================================================
# 安装
# ============================================================================

install(TARGETS phoenix_platform
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/phoenix/platform
    DESTINATION include/phoenix
    FILES_MATCHING PATTERN "*.hpp"
)

# ============================================================================
# 构建摘要
# ============================================================================

message(STATUS "")
message(STATUS "Phoenix Platform Layer:")
message(STATUS "  Platform: ${PHOENIX_PLATFORM_NAME}")
message(STATUS "  Sources: ${PHOENIX_PLATFORM_SOURCES}")
message(STATUS "  Libraries: ${PHOENIX_PLATFORM_LIBS}")
if(APPLE)
message(STATUS "  Frameworks: ${PHOENIX_PLATFORM_FRAMEWORKS}")
endif()
message(STATUS "")
