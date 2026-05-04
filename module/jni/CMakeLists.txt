cmake_minimum_required(VERSION 3.10)
project(MtkSpoof)

set(CMAKE_CXX_STANDARD 17)

# Inclui os cabeçalhos locais
include_directories(.)

# Configura o Dobby como um subprojeto CMake
add_subdirectory(dobby)

# Nosso módulo nativo
add_library(mtk_spoof SHARED main.cpp vulkan_hook.cpp)

# Adiciona o diretório de inclusão do Dobby
target_include_directories(mtk_spoof PRIVATE dobby/include)

# Link com o log, libdl, e a biblioteca dobby
target_link_libraries(mtk_spoof log dl dobby)
