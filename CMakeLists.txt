cmake_minimum_required(VERSION 3.20)
project(AstronEngine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8 /W3)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    add_compile_options(-Wall -Wextra)
endif()

include(FetchContent)
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

# -------------------- GLFW 3.4 --------------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(glfw)

# -------------------- Dear ImGui v1.91.8-docking --------------------
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8-docking.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(astron_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(astron_imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

add_library(astron_imgui_backends STATIC
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_compile_definitions(astron_imgui_backends PUBLIC GLFW_INCLUDE_NONE)
target_link_libraries(astron_imgui_backends PUBLIC astron_imgui glfw)

# -------------------- C11 Core Library --------------------
add_library(astron_core_c STATIC
    engine/core_c/astron_math.c
    engine/core_c/astron_gl.c
    engine/core_c/astron_platform.c
)
target_include_directories(astron_core_c PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/core_c
)
if(WIN32)
    target_link_libraries(astron_core_c PUBLIC psapi shell32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(astron_core_c PUBLIC m)
endif()

# -------------------- C++20 Engine Core + OpenGL 3.3 + UI --------------------
add_library(astron_engine_lib STATIC
    engine/src/Json.cpp
    engine/src/HttpClient.cpp
    engine/src/AIProviders.cpp
    engine/src/Scene.cpp
    engine/src/ProjectManager.cpp
    engine/src/AIAgent.cpp
    engine/src/TextureManager.cpp
    engine/src/Viewport3D.cpp
    engine/src/UIWidgets.cpp
    engine/src/ScreenSplashLogin.cpp
    engine/src/ScreenHomeBuild.cpp
    engine/src/ScreenEditor.cpp
    engine/src/EngineApp.cpp
)

target_include_directories(astron_engine_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/include
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
)
target_compile_definitions(astron_engine_lib PUBLIC GLFW_INCLUDE_NONE)
target_link_libraries(astron_engine_lib PUBLIC
    astron_core_c
    astron_imgui
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

if(WIN32)
    target_link_libraries(astron_engine_lib PUBLIC winhttp opengl32)
else()
    find_package(CURL QUIET)
    if(CURL_FOUND)
        target_compile_definitions(astron_engine_lib PRIVATE ASTRON_USE_LIBCURL=1)
        target_link_libraries(astron_engine_lib PUBLIC CURL::libcurl)
    endif()
    target_link_libraries(astron_engine_lib PUBLIC OpenGL::GL)
endif()

# -------------------- Native Desktop Astron Engine Executable --------------------
add_executable(AstronEngine WIN32
    engine/src/main.cpp
)
target_link_libraries(AstronEngine PRIVATE
    astron_engine_lib
    astron_imgui_backends
    glfw
)
if(MSVC)
    target_link_options(AstronEngine PRIVATE /ENTRY:mainCRTStartup)
endif()

add_custom_command(TARGET AstronEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:AstronEngine>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:AstronEngine>/shaders"
)

# -------------------- Native C++20 / C11 Test Suite --------------------
option(ASTRON_BUILD_TESTS "Build Astron Engine native test suite" ON)
if(ASTRON_BUILD_TESTS)
    enable_testing()
    add_executable(astron_engine_tests
        engine/tests/engine_tests.cpp
    )
    target_link_libraries(astron_engine_tests PRIVATE
        astron_engine_lib
    )
    add_test(
        NAME AstronEngineNativeTests
        COMMAND astron_engine_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
