cmake_minimum_required(VERSION 3.10)
project(MtkSpoof)

set(CMAKE_CXX_STANDARD 17)

# Inclui os cabeçalhos locais
include_directories(.)

# Nosso módulo nativo
add_library(mtk_spoof SHARED main.cpp vulkan_hook.cpp And64InlineHook.cpp)

# Link com o log e libdl
target_link_libraries(mtk_spoof log dl)
