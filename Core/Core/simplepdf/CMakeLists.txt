cmake_minimum_required(VERSION 3.20)

project(simplepdf
    VERSION 0.1.0
    DESCRIPTION "Small C++ PDF helper built on MuPDF and Cairo"
    LANGUAGES C CXX)

option(SIMPLEPDF_BUILD_EXAMPLE "Build the SimplePDF example" ON)

find_package(unofficial-libmupdf CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)

add_library(simplepdf
    src/Document.cpp)

add_library(simplepdf::simplepdf ALIAS simplepdf)

target_compile_features(simplepdf PUBLIC cxx_std_17)

target_include_directories(simplepdf
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(simplepdf
    PUBLIC
        unofficial::libmupdf::libmupdf
        PkgConfig::CAIRO)

if(MSVC)
    target_compile_options(simplepdf PRIVATE /W4 /permissive-)
else()
    target_compile_options(simplepdf PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(SIMPLEPDF_BUILD_EXAMPLE)
    add_executable(simplepdf_example examples/main.cpp)
    target_link_libraries(simplepdf_example PRIVATE simplepdf::simplepdf)
endif()

