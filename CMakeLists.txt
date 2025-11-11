cmake_minimum_required(VERSION 3.19)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" VER_RAW)
string(STRIP ${VER_RAW} HYPRLAUNCHER_VERSION)

add_compile_definitions(HYPRLAUNCHER_VERSION="${HYPRLAUNCHER_VERSION}")

project(
  hyprlauncher
  VERSION ${HYPRLAUNCHER_VERSION}
  DESCRIPTION "A multipurpose and versatile launcher / picker for Hyprland")

include(CTest)
include(CheckIncludeFile)
include(GNUInstallDirs)

set(PREFIX ${CMAKE_INSTALL_PREFIX})
set(INCLUDE ${CMAKE_INSTALL_FULL_INCLUDEDIR})
set(LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(
  deps
  REQUIRED
  IMPORTED_TARGET
  hyprtoolkit
  pixman-1
  libdrm
  hyprutils
  hyprwire
  icu-uc
  hyprlang
  libqalculate
)

set(CMAKE_CXX_STANDARD 23)
add_compile_options(
  -Wall
  -Wextra
  -Wno-unused-parameter
  -Wno-unused-value
  -Wno-missing-field-initializers
  -Wpedantic)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES DEBUG)
  message(STATUS "Configuring hyprlauncher in Debug")
  add_compile_definitions(HYPRLAUNCHER_DEBUG)
else()
  add_compile_options(-O3)
  message(STATUS "Configuring hyprlauncher in Release")
endif()

file(GLOB_RECURSE SRCFILES CONFIGURE_DEPENDS "src/*.cpp" "protocols/*.cpp" "include/*.hpp")

add_executable(hyprlauncher ${SRCFILES})

function(hyprprotocol protoPath protoName)
  set(path ${CMAKE_SOURCE_DIR}/${protoPath})
  add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/protocols/${protoName}-client.cpp
           ${CMAKE_SOURCE_DIR}/protocols/${protoName}-client.hpp
           ${CMAKE_SOURCE_DIR}/protocols/${protoName}-spec.hpp
    COMMAND hyprwire-scanner --client ${path}/${protoName}.xml
            ${CMAKE_SOURCE_DIR}/protocols/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  target_sources(hyprlauncher PRIVATE protocols/${protoName}-client.cpp
                                   protocols/${protoName}-client.hpp
                                   protocols/${protoName}-spec.hpp)
endfunction()

function(hyprprotocolServer protoPath protoName)
  set(path ${CMAKE_SOURCE_DIR}/${protoPath})
  add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/protocols/${protoName}-server.cpp
           ${CMAKE_SOURCE_DIR}/protocols/${protoName}-server.hpp
    COMMAND hyprwire-scanner ${path}/${protoName}.xml
            ${CMAKE_SOURCE_DIR}/protocols/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  target_sources(hyprlauncher PRIVATE protocols/${protoName}-server.cpp
                                   protocols/${protoName}-server.hpp
                                   )
endfunction()

hyprprotocol(protocols hyprlauncher_core)
hyprprotocolServer(protocols hyprlauncher_core)

target_include_directories(hyprlauncher PRIVATE "./protocols")

target_link_libraries(hyprlauncher PkgConfig::deps)

install(TARGETS hyprlauncher)
