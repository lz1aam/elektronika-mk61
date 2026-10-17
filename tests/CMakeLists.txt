set(PMK_SOURCE_DIR "${PROJECT_SOURCE_DIR}")

function(mk61_test_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_executable(pmk-hardware-io-test ${CMAKE_CURRENT_LIST_DIR}/hardware_io.cpp)
target_link_libraries(pmk-hardware-io-test PRIVATE pmk_hardware)

add_executable(pmk-core-smoke ${CMAKE_CURRENT_LIST_DIR}/core_smoke.cpp)
target_link_libraries(pmk-core-smoke PRIVATE pmk_core)

add_executable(pmk-program-regression-test
    ${CMAKE_CURRENT_LIST_DIR}/program_regression.cpp)
target_link_libraries(pmk-program-regression-test PRIVATE pmk_core)

add_executable(pmk-matrix-probe ${CMAKE_CURRENT_LIST_DIR}/matrix_probe.cpp)
target_link_libraries(pmk-matrix-probe PRIVATE pmk_core)

add_executable(pmk-semantics-probe ${CMAKE_CURRENT_LIST_DIR}/semantics_probe.cpp)
target_link_libraries(pmk-semantics-probe PRIVATE pmk_core)

add_test(NAME core-microcode-smoke COMMAND pmk-core-smoke)
add_test(NAME real-program-regression COMMAND pmk-program-regression-test)
add_test(NAME hardware-io COMMAND pmk-hardware-io-test)

add_executable(pmk-timing-test ${CMAKE_CURRENT_LIST_DIR}/timing.cpp)
target_link_libraries(pmk-timing-test PRIVATE pmk_core)
add_test(NAME portable-clock-and-batching COMMAND pmk-timing-test)

add_executable(pmk-state-test ${CMAKE_CURRENT_LIST_DIR}/state.cpp)
target_link_libraries(pmk-state-test PRIVATE pmk_core)
add_test(NAME portable-full-state COMMAND pmk-state-test)

add_executable(pmk-keyboard-phase-test ${CMAKE_CURRENT_LIST_DIR}/keyboard_phase.cpp)
target_link_libraries(pmk-keyboard-phase-test PRIVATE pmk_core)
add_test(NAME keyboard-word-phase COMMAND pmk-keyboard-phase-test)

foreach(target pmk-hardware-io-test pmk-core-smoke pmk-program-regression-test
        pmk-matrix-probe pmk-semantics-probe pmk-timing-test pmk-state-test
        pmk-keyboard-phase-test)
    mk61_test_warnings(${target})
endforeach()
set_tests_properties(core-microcode-smoke real-program-regression hardware-io
    portable-clock-and-batching portable-full-state keyboard-word-phase PROPERTIES
    LABELS "core;portable" TIMEOUT 120)

# Only the Qt-independent core is exercised on Windows and macOS for now.
if(NOT TARGET elektronika-mk61 OR NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    return()
endif()
find_package(Qt6 6.4 COMPONENTS Test REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt_add_executable(pmk-vfd-render-test
    ${CMAKE_CURRENT_LIST_DIR}/vfd_render.cpp
    ${PMK_SOURCE_DIR}/src/ui/vfd_display.cpp
    ${PMK_SOURCE_DIR}/src/ui/vfd_display.hpp
)
target_include_directories(pmk-vfd-render-test PRIVATE ${PMK_SOURCE_DIR}/src/ui)
target_link_libraries(pmk-vfd-render-test PRIVATE pmk_core Qt6::Widgets)
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
    ${CMAKE_CURRENT_LIST_DIR}/panel_symbols.cpp
    ${PMK_SOURCE_DIR}/src/ui/panel_symbols.cpp
    ${PMK_SOURCE_DIR}/src/ui/panel_symbols.hpp
    ${PMK_SOURCE_DIR}/resources/elektronika-mk61.qrc
)
target_include_directories(pmk-panel-symbol-test PRIVATE ${PMK_SOURCE_DIR}/src/ui)
target_link_libraries(pmk-panel-symbol-test PRIVATE Qt6::Widgets)
add_test(NAME panel-symbols
    COMMAND pmk-panel-symbol-test
            ${CMAKE_CURRENT_BINARY_DIR}/pmk-panel-symbol-test.png)
set_tests_properties(panel-symbols PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)

qt_add_executable(pmk-gui-interaction-test
    ${CMAKE_CURRENT_LIST_DIR}/gui_interaction.cpp
    ${PMK_SOURCE_DIR}/src/ui/calculator_widget.cpp
    ${PMK_SOURCE_DIR}/src/ui/calculator_widget.hpp
    ${PMK_SOURCE_DIR}/src/ui/panel_symbols.cpp
    ${PMK_SOURCE_DIR}/src/ui/panel_symbols.hpp
    ${PMK_SOURCE_DIR}/src/ui/vfd_display.cpp
    ${PMK_SOURCE_DIR}/src/ui/vfd_display.hpp
    ${PMK_SOURCE_DIR}/resources/elektronika-mk61.qrc
)
target_include_directories(pmk-gui-interaction-test PRIVATE ${PMK_SOURCE_DIR}/src/ui)
target_link_libraries(pmk-gui-interaction-test PRIVATE
    pmk_core Qt6::Widgets Qt6::Test)
add_test(NAME gui-interaction COMMAND pmk-gui-interaction-test)
set_tests_properties(gui-interaction PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)

qt_add_executable(pmk-native-state-ui-test
    ${CMAKE_CURRENT_LIST_DIR}/native_state_ui.cpp
    ${PMK_SOURCE_DIR}/src/ui/calculator_widget.cpp
    ${PMK_SOURCE_DIR}/src/ui/calculator_widget.hpp
    ${PMK_SOURCE_DIR}/src/ui/panel_symbols.cpp
    ${PMK_SOURCE_DIR}/src/ui/vfd_display.cpp
    ${PMK_SOURCE_DIR}/resources/elektronika-mk61.qrc)
target_include_directories(pmk-native-state-ui-test PRIVATE ${PMK_SOURCE_DIR}/src/ui)
target_link_libraries(pmk-native-state-ui-test PRIVATE pmk_core Qt6::Widgets Qt6::Test)
add_test(NAME native-state-ui COMMAND pmk-native-state-ui-test)
set_tests_properties(native-state-ui PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 10)

qt_add_executable(pmk-stack-gui-probe
    ${CMAKE_CURRENT_LIST_DIR}/stack_gui_probe.cpp
    ${PMK_SOURCE_DIR}/src/ui/calculator_widget.cpp
    ${PMK_SOURCE_DIR}/src/ui/calculator_widget.hpp
    ${PMK_SOURCE_DIR}/src/ui/panel_symbols.cpp
    ${PMK_SOURCE_DIR}/src/ui/panel_symbols.hpp
    ${PMK_SOURCE_DIR}/src/ui/vfd_display.cpp
    ${PMK_SOURCE_DIR}/src/ui/vfd_display.hpp
    ${PMK_SOURCE_DIR}/resources/elektronika-mk61.qrc
)
target_include_directories(pmk-stack-gui-probe PRIVATE ${PMK_SOURCE_DIR}/src/ui)
target_link_libraries(pmk-stack-gui-probe PRIVATE
    pmk_core Qt6::Widgets Qt6::Test)
add_test(NAME rapid-expanded-stack-input COMMAND pmk-stack-gui-probe)
set_tests_properties(rapid-expanded-stack-input PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 20
)

add_test(NAME gui-startup-smoke COMMAND elektronika-mk61 --smoke-test)
set_tests_properties(gui-startup-smoke PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)
add_test(NAME gui-render-smoke
    COMMAND elektronika-mk61 --render
            ${CMAKE_CURRENT_BINARY_DIR}/pmk-render-smoke.png)
set_tests_properties(gui-render-smoke PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)
add_test(NAME gui-render-hidpi-smoke
    COMMAND elektronika-mk61 --render
            ${CMAKE_CURRENT_BINARY_DIR}/pmk-render-2x-smoke.png)
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

foreach(target pmk-vfd-render-test pmk-panel-symbol-test pmk-gui-interaction-test
        pmk-native-state-ui-test pmk-stack-gui-probe)
    mk61_test_warnings(${target})
endforeach()
set_tests_properties(vfd-render panel-symbols gui-interaction native-state-ui
    rapid-expanded-stack-input gui-startup-smoke gui-render-smoke
    gui-render-hidpi-smoke gui-render-stack-smoke PROPERTIES LABELS "gui")
