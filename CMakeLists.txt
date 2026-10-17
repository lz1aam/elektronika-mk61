cmake_minimum_required(VERSION 3.20)
project(elektronika-mk61 VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
option(MK61_BUILD_GUI "Build the Qt desktop application" ON)
option(MK61_INSTALL_DEVELOPMENT "Install core libraries, headers and CMake package" OFF)
option(MK61_ENABLE_PACKAGING "Configure platform packages with CPack" OFF)
option(MK61_DEPLOY_QT "Bundle Qt runtime (Windows/macOS only)" OFF)

add_library(pmk_hardware STATIC
    src/hardware/calculator_io.cpp
)
add_library(MK61::Hardware ALIAS pmk_hardware)
set_target_properties(pmk_hardware PROPERTIES EXPORT_NAME Hardware
    POSITION_INDEPENDENT_CODE ON)
target_include_directories(pmk_hardware PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/hardware>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mk61>)
target_compile_features(pmk_hardware PUBLIC cxx_std_20)

add_library(pmk_core STATIC
    src/core/calculator_core.cpp
    src/core/core_state.cpp
    src/core/delay_memory.cpp
    src/core/microcode_chip.cpp
    src/core/program_text.cpp
)
add_library(MK61::Core ALIAS pmk_core)
set_target_properties(pmk_core PROPERTIES EXPORT_NAME Core
    POSITION_INDEPENDENT_CODE ON)
target_include_directories(pmk_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/core>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mk61>)
target_compile_features(pmk_core PUBLIC cxx_std_20)
target_link_libraries(pmk_core PUBLIC pmk_hardware)
foreach(target pmk_core pmk_hardware)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

if(MK61_INSTALL_DEVELOPMENT)
    install(TARGETS pmk_core pmk_hardware EXPORT MK61Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES
        src/core/calculator_core.hpp src/core/microcode_chip.hpp
        src/core/delay_memory.hpp src/core/keymap.hpp src/core/program_text.hpp
        src/core/realtime_clock.hpp src/hardware/calculator_io.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mk61)
    install(EXPORT MK61Targets NAMESPACE MK61::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MK61)
    configure_package_config_file(packaging/MK61Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/MK61Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MK61)
    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/MK61ConfigVersion.cmake
        VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MK61Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/MK61ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MK61)
endif()

install(FILES LICENSE NOTICE resources/fonts/DejaVu-LICENSE.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/${PROJECT_NAME})

if(MK61_BUILD_GUI)
    find_package(Qt6 6.4 COMPONENTS Widgets REQUIRED)
    qt_standard_project_setup()
    set(CMAKE_AUTORCC ON)
    qt_add_executable(elektronika-mk61 WIN32 MACOSX_BUNDLE
        src/app/main.cpp
        src/ui/calculator_widget.cpp
        src/ui/calculator_widget.hpp
        src/ui/panel_symbols.cpp
        src/ui/panel_symbols.hpp
        src/ui/vfd_display.cpp
        src/ui/vfd_display.hpp
        resources/elektronika-mk61.qrc
    )
    target_include_directories(elektronika-mk61 PRIVATE src/ui)
    target_compile_definitions(elektronika-mk61 PRIVATE
        PMK_VERSION="${PROJECT_VERSION}")
    if(MSVC)
        target_compile_options(elektronika-mk61 PRIVATE /utf-8)
    endif()
    set_target_properties(elektronika-mk61 PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.elektronika.mk61"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    target_link_libraries(elektronika-mk61 PRIVATE pmk_core Qt6::Widgets)
    install(TARGETS elektronika-mk61
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} BUNDLE DESTINATION .)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        configure_file(packaging/elektronika-mk61.desktop.in
            ${CMAKE_CURRENT_BINARY_DIR}/elektronika-mk61.desktop.in COPYONLY)
        configure_file(packaging/install-desktop.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/install-desktop.cmake @ONLY)
        install(FILES resources/icons/elektronika-mk61.svg
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
        install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/install-desktop.cmake)
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(MK61_ENABLE_PACKAGING)
    include(packaging/Packages.cmake)
elseif(MK61_DEPLOY_QT)
    message(FATAL_ERROR "MK61_DEPLOY_QT requires MK61_ENABLE_PACKAGING")
endif()

