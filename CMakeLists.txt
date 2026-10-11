cmake_minimum_required(VERSION 4.0)

project(json3 LANGUAGES C)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE main.c json3.c)
target_compile_features(${PROJECT_NAME} PRIVATE c_std_23)
