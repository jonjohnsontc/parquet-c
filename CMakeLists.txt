cmake_minimum_required(VERSION 3.29)
project(parquet_c C)

set(CMAKE_C_STANDARD 11)

add_executable(parquet_c
    src/main.c)