cmake_minimum_required(VERSION 3.15)
project(hdcpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(hdcpp INTERFACE)
target_include_directories(hdcpp INTERFACE include)

# Compile features: popcount detection, optimisation flags (optional)
target_compile_features(hdcpp INTERFACE cxx_std_17)

# Optional: define a macro for release builds
target_compile_definitions(hdcpp INTERFACE $<$<CONFIG:RELEASE>:NDEBUG>)

# Install rules
include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS hdcpp EXPORT hdcppTargets)
install(EXPORT hdcppTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hdcpp)
