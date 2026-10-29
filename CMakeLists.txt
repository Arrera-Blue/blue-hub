cmake_minimum_required(VERSION 3.20)

project(blue-hub
    VERSION 2026.1.0
    DESCRIPTION "Arrera Blue Hub - Gestionnaire d'applications"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -pedantic)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)
pkg_check_modules(ADWAITA REQUIRED IMPORTED_TARGET libadwaita-1)

add_executable(blue-hub
    src/main.cpp
    src/ui/main_window.cpp
    src/ui/discover_page.cpp
    src/ui/installed_page.cpp
    src/ui/updates_page.cpp
)

target_include_directories(blue-hub PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(blue-hub PRIVATE
    PkgConfig::GTKMM
    PkgConfig::ADWAITA
)