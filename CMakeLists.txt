cmake_minimum_required(VERSION 3.15)
project(CursorMotionBlur VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 Windows 编译选项
if(WIN32)
    # 启用 Unicode
    add_definitions(-DUNICODE -D_UNICODE)
    
    # 禁用一些警告
    if(MSVC)
        add_definitions(/wd4996)  # 禁用不安全函数警告
    endif()
endif()

# 源文件目录
set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Core")
set(CURSORHOOK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CursorHook")
set(CUROVERLAY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CursorOverlay")
set(MAIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Main")

# ==================== Core 库 ====================
add_library(Core STATIC
    ${CORE_DIR}/MotionBlur.cpp
)

target_include_directories(Core PUBLIC
    ${CORE_DIR}
)

# ==================== CursorHook DLL ====================
add_library(CursorHook SHARED
    ${CURSORHOOK_DIR}/MouseHook.cpp
)

target_include_directories(CursorHook PRIVATE
    ${CURSORHOOK_DIR}
    ${CORE_DIR}
)

target_link_libraries(CursorHook PRIVATE
    Core
)

# 链接 Windows 库
target_link_libraries(CursorHook PRIVATE
    user32
)

# ==================== CursorOverlay 库 ====================
add_library(CursorOverlay SHARED
    ${CUROVERLAY_DIR}/OverlayWindow.cpp
)

target_include_directories(CursorOverlay PUBLIC
    ${CUROVERLAY_DIR}
    ${CORE_DIR}
    ${CURSORHOOK_DIR}
)

target_link_libraries(CursorOverlay PUBLIC
    Core
    CursorHook
    user32
    gdi32
    msimg32
)

# ==================== Main 可执行文件 ====================
add_executable(CursorMotionBlur WIN32
    ${MAIN_DIR}/main.cpp
)

target_include_directories(CursorMotionBlur PRIVATE
    ${CORE_DIR}
    ${CURSORHOOK_DIR}
    ${CUROVERLAY_DIR}
)

target_link_libraries(CursorMotionBlur PRIVATE
    CursorOverlay
    CursorHook
    user32
    gdi32
)

# 复制 DLL 到输出目录
add_custom_command(TARGET CursorMotionBlur POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:CursorHook>
        $<TARGET_FILE_DIR:CursorMotionBlur>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:CursorOverlay>
        $<TARGET_FILE_DIR:CursorMotionBlur>
    COMMENT "Copying DLLs to output directory"
)

# ==================== 安装规则（可选）====================
install(TARGETS CursorMotionBlur CursorHook
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

# ==================== 输出信息 ====================
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")