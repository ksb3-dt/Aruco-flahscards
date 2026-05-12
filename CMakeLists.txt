cmake_minimum_required(VERSION 3.16)
project(ARFlashcards LANGUAGES CXX)

# C++17, no GNU extensions
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Export compile_commands.json for clangd / IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Strict warnings on GCC / Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

# OpenCV — requires opencv_contrib for the aruco module
find_package(OpenCV 4 REQUIRED
    COMPONENTS core imgproc imgcodecs videoio highgui aruco calib3d
)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)
pkg_check_modules(ASSIMP REQUIRED IMPORTED_TARGET assimp)

message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libs:    ${OpenCV_LIBS}")

# Glob all sources under src/ (CONFIGURE_DEPENDS = re-glob on rebuild)
file(GLOB AR_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(ar_flashcards ${AR_SOURCES})

target_include_directories(ar_flashcards PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(ar_flashcards PRIVATE
    ${OpenCV_LIBS}
    OpenGL::GL
    PkgConfig::SDL2
    PkgConfig::GLEW
    PkgConfig::ASSIMP
)
