cmake_minimum_required(VERSION 3.8)

project(SubstDriveGuard LANGUAGES CXX)

add_library(SubstDriveGuard INTERFACE)

target_include_directories(SubstDriveGuard INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR} 
)