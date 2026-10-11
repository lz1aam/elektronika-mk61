cmake_minimum_required(VERSION 3.20)
project(elektronika-mk61 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

set(PMK_LOCAL_TESTS OFF)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    set(PMK_LOCAL_TESTS ON)
endif()

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

if(PMK_LOCAL_TESTS)
    add_executable(pmk-hardware-io-test tests/hardware_io.cpp)
    target_link_libraries(pmk-hardware-io-test PRIVATE pmk_hardware)

    add_executable(pmk-core-smoke tests/core_smoke.cpp)
    target_link_libraries(pmk-core-smoke PRIVATE pmk_core)

    add_executable(pmk-matrix-probe tests/matrix_probe.cpp)
    target_link_libraries(pmk-matrix-probe PRIVATE pmk_core)

    add_executable(pmk-semantics-probe tests/semantics_probe.cpp)
    target_link_libraries(pmk-semantics-probe PRIVATE pmk_core)

    add_test(NAME core-microcode-smoke COMMAND pmk-core-smoke)
    add_test(NAME hardware-io COMMAND pmk-hardware-io-test)
endif()

set(pmk_qt_components Widgets)
if(PMK_LOCAL_TESTS)
    list(APPEND pmk_qt_components Test)
endif()
find_package(Qt6 6.4 COMPONENTS ${pmk_qt_components} QUIET)
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
    target_link_libraries(elektronika-mk61 PRIVATE pmk_core Qt6::Widgets)
    if(PMK_LOCAL_TESTS)
        qt_add_executable(pmk-vfd-render-test
            tests/vfd_render.cpp
            src/ui/vfd_display.cpp
            src/ui/vfd_display.hpp
        )
        target_include_directories(pmk-vfd-render-test PRIVATE src/ui)
        target_link_libraries(pmk-vfd-render-test PRIVATE
            pmk_hardware Qt6::Widgets)
        add_test(NAME vfd-render
            COMMAND pmk-vfd-render-test
                    ${CMAKE_CURRENT_BINARY_DIR}/pmk-vfd-segment-test.png
                    ${CMAKE_CURRENT_BINARY_DIR}/pmk-vfd-error-test.png
                    ${CMAKE_CURRENT_BINARY_DIR}/pmk-vfd-label-test.png)
        set_tests_properties(vfd-render PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 10
        )
        qt_add_executable(pmk-panel-symbol-test
            tests/panel_symbols.cpp
            src/ui/panel_symbols.cpp
            src/ui/panel_symbols.hpp
            resources/elektronika-mk61.qrc
        )
        target_include_directories(pmk-panel-symbol-test PRIVATE src/ui)
        target_link_libraries(pmk-panel-symbol-test PRIVATE Qt6::Widgets)
        add_test(NAME panel-symbols
            COMMAND pmk-panel-symbol-test
                    ${CMAKE_CURRENT_BINARY_DIR}/pmk-panel-symbol-test.png)
        set_tests_properties(panel-symbols PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 10
        )
        qt_add_executable(pmk-gui-interaction-test
            tests/gui_interaction.cpp
            src/ui/calculator_widget.cpp
            src/ui/calculator_widget.hpp
            src/ui/panel_symbols.cpp
            src/ui/panel_symbols.hpp
            src/ui/vfd_display.cpp
            src/ui/vfd_display.hpp
            resources/elektronika-mk61.qrc
        )
        target_include_directories(pmk-gui-interaction-test PRIVATE src/ui)
        target_link_libraries(pmk-gui-interaction-test PRIVATE
            pmk_core Qt6::Widgets Qt6::Test)
        add_test(NAME gui-interaction COMMAND pmk-gui-interaction-test)
        set_tests_properties(gui-interaction PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 10
        )
        add_test(NAME gui-startup-smoke COMMAND elektronika-mk61 --smoke-test)
        set_tests_properties(gui-startup-smoke PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 10
        )
        add_test(NAME gui-render-smoke
            COMMAND elektronika-mk61 --render ${CMAKE_CURRENT_BINARY_DIR}/pmk-render-smoke.png)
        set_tests_properties(gui-render-smoke PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 10
        )
        add_test(NAME gui-render-hidpi-smoke
            COMMAND elektronika-mk61 --render ${CMAKE_CURRENT_BINARY_DIR}/pmk-render-2x-smoke.png)
        set_tests_properties(gui-render-hidpi-smoke PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=2"
            TIMEOUT 10
        )
        add_test(NAME gui-render-stack-smoke
            COMMAND elektronika-mk61 --render-stack
                    ${CMAKE_CURRENT_BINARY_DIR}/pmk-stack-display.png)
        set_tests_properties(gui-render-stack-smoke PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 10
        )
    endif()
    install(TARGETS elektronika-mk61 RUNTIME DESTINATION bin)
    install(FILES packaging/elektronika-mk61.desktop
        DESTINATION share/applications)
    install(FILES resources/icons/elektronika-mk61.svg
        DESTINATION share/icons/hicolor/scalable/apps)
else()
    message(WARNING "Qt 6 Widgets development package not found; native GUI target is disabled")
endif()

