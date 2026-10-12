cmake_minimum_required(VERSION 3.20)
project(elektronika-mk61 VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

configure_file(
    packaging/elektronika-mk61.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/elektronika-mk61.desktop.in
    COPYONLY
)
configure_file(
    packaging/install-desktop.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/install-desktop.cmake
    @ONLY
)

add_library(pmk_hardware STATIC
    src/hardware/calculator_io.cpp
)
target_include_directories(pmk_hardware PUBLIC src/hardware)
target_compile_options(pmk_hardware PRIVATE -Wall -Wextra -Wpedantic)

add_library(pmk_core STATIC
    src/core/calculator_core.cpp
    src/core/delay_memory.cpp
    src/core/microcode_chip.cpp
    src/core/program_text.cpp
)
target_include_directories(pmk_core PUBLIC src/core)
target_link_libraries(pmk_core PUBLIC pmk_hardware)
target_compile_options(pmk_core PRIVATE -Wall -Wextra -Wpedantic)

find_package(Qt6 6.4 COMPONENTS Widgets QUIET)
if(Qt6Widgets_FOUND)
    qt_standard_project_setup()
    set(CMAKE_AUTORCC ON)
    qt_add_executable(elektronika-mk61
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
    target_link_libraries(elektronika-mk61 PRIVATE pmk_core Qt6::Widgets)
    install(TARGETS elektronika-mk61 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES resources/icons/elektronika-mk61.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
    install(FILES LICENSE NOTICE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/${PROJECT_NAME})
    install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/install-desktop.cmake)
else()
    message(WARNING "Qt 6 Widgets development package not found; native GUI target is disabled")
endif()

