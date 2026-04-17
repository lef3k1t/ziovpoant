cmake_minimum_required(VERSION 3.20)

project(ZIOVPOANT VERSION 1.0.0 LANGUAGES CXX)

add_executable(ZIOVPOANT WIN32
    src/main.cpp
)

target_compile_features(ZIOVPOANT PRIVATE cxx_std_20)

target_compile_definitions(ZIOVPOANT PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(ZIOVPOANT PRIVATE
    user32
    gdi32
    shell32
)

install(TARGETS ZIOVPOANT RUNTIME DESTINATION .)
