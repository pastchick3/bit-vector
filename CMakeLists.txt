cmake_minimum_required(VERSION 3.15)

project(bit-vector)

add_subdirectory(src)
add_subdirectory(tests)

add_compile_options(-Wall)
target_link_libraries(test-bit-vector m)
target_include_directories(test-bit-vector PUBLIC
    "${PROJECT_SOURCE_DIR}/tests/Unity-2.5.2"
    "${PROJECT_SOURCE_DIR}/include"
)
