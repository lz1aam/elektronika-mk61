# Binary packages contain the desktop application, not the development SDK.
if(NOT TARGET elektronika-mk61 OR MK61_INSTALL_DEVELOPMENT)
    message(FATAL_ERROR "Desktop packages require MK61_BUILD_GUI=ON and MK61_INSTALL_DEVELOPMENT=OFF")
endif()

set(CPACK_PACKAGE_NAME "elektronika-mk61")
set(CPACK_PACKAGE_VENDOR "Elektronika MK-61 project")
set(CPACK_PACKAGE_CONTACT "https://github.com/lz1aam/elektronika-mk61/issues")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/lz1aam/elektronika-mk61")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Elektronika MK-61 programmable RPN calculator emulator")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_BINARY_DIR}/packages")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_VERBATIM_VARIABLES ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr")
        message(FATAL_ERROR "Native Linux packages require CMAKE_INSTALL_PREFIX=/usr; use a separate build directory")
    endif()
    if(MK61_DEPLOY_QT)
        message(FATAL_ERROR "Native Linux packages use distribution Qt libraries; do not bundle Qt")
    endif()
    # CPack stages through DESTDIR. Keep /usr in launchers, never a staging path.
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
    set(CPACK_SET_DESTDIR ON)
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_SECTION "science")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    # SVG is loaded as a plugin, so ELF dependency scanning cannot discover it.
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6svg6")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Elektronika MK-61 project <lz1aam@users.noreply.github.com>")
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
    set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-only")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
    set(CPACK_RPM_PACKAGE_AUTOREQ ON)
    set(CPACK_RPM_PACKAGE_RELOCATABLE OFF)
    # Fedora and openSUSE use different names for the dynamically loaded plugin.
    set(MK61_RPM_QT_SVG_PACKAGE "qt6-qtsvg" CACHE STRING "Distribution package providing Qt's SVG plugin")
    set(CPACK_RPM_PACKAGE_REQUIRES "${MK61_RPM_QT_SVG_PACKAGE}")
    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        /usr/share/applications /usr/share/icons /usr/share/icons/hicolor
        /usr/share/icons/hicolor/scalable /usr/share/icons/hicolor/scalable/apps)
elseif(WIN32 OR APPLE)
    if(NOT MK61_DEPLOY_QT)
        message(FATAL_ERROR "Windows/macOS packages require MK61_DEPLOY_QT=ON")
    endif()
    find_package(Qt6 6.4 COMPONENTS Svg REQUIRED)
    # Rendering the existing SVG is a build tool, not an application redesign.
    qt_add_executable(mk61-package-icons packaging/package_icons.cpp)
    target_link_libraries(mk61-package-icons PRIVATE Qt6::Gui Qt6::Svg)
    set(icon_dir "${PROJECT_BINARY_DIR}/package-icons")
    set(icon_file "${icon_dir}/elektronika-mk61.ico")
    add_custom_command(OUTPUT "${icon_file}" "${icon_dir}/iconset.stamp"
        COMMAND ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=offscreen
            $<TARGET_FILE:mk61-package-icons>
            "${PROJECT_SOURCE_DIR}/resources/icons/elektronika-mk61.svg" "${icon_dir}"
        DEPENDS mk61-package-icons resources/icons/elektronika-mk61.svg
        VERBATIM)
    if(WIN32)
        enable_language(RC)
        configure_file("${CMAKE_CURRENT_LIST_DIR}/windows.rc.in"
            "${PROJECT_BINARY_DIR}/windows.rc" @ONLY)
        set_source_files_properties("${PROJECT_BINARY_DIR}/windows.rc"
            PROPERTIES OBJECT_DEPENDS "${icon_file}")
        add_custom_target(mk61-native-icon DEPENDS "${icon_file}")
        add_dependencies(elektronika-mk61 mk61-native-icon)
        target_sources(elektronika-mk61 PRIVATE "${PROJECT_BINARY_DIR}/windows.rc")
        set(CPACK_GENERATOR "ZIP;NSIS")
        set(CPACK_PACKAGE_INSTALL_DIRECTORY "Elektronika MK-61")
        set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "Elektronika MK-61")
        set(CPACK_PACKAGE_EXECUTABLES "elektronika-mk61;Elektronika MK-61")
        set(CPACK_NSIS_DISPLAY_NAME "Elektronika MK-61")
        set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\elektronika-mk61.exe")
        set(CPACK_NSIS_MUI_ICON "${icon_file}")
        set(CPACK_NSIS_MUI_UNIICON "${icon_file}")
        set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
        set(CPACK_NSIS_MODIFY_PATH OFF)
    else()
        find_program(MK61_ICONUTIL iconutil REQUIRED)
        set(icns_file "${icon_dir}/elektronika-mk61.icns")
        add_custom_command(OUTPUT "${icns_file}"
            COMMAND "${MK61_ICONUTIL}" -c icns
                "${icon_dir}/elektronika-mk61.iconset" -o "${icns_file}"
            DEPENDS "${icon_dir}/iconset.stamp" VERBATIM)
        target_sources(elektronika-mk61 PRIVATE "${icns_file}")
        set_source_files_properties("${icns_file}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        set_target_properties(elektronika-mk61 PROPERTIES
            MACOSX_BUNDLE_ICON_FILE "elektronika-mk61.icns"
            MACOSX_BUNDLE_BUNDLE_NAME "Elektronika MK-61")
        # Keep notices inside the app copied from the DMG, not beside it.
        install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" "${PROJECT_SOURCE_DIR}/NOTICE"
            "${PROJECT_SOURCE_DIR}/resources/fonts/DejaVu-LICENSE.txt"
            DESTINATION "elektronika-mk61.app/Contents/Resources/licenses")
        set(CPACK_GENERATOR "DragNDrop")
        set(CPACK_DMG_VOLUME_NAME "Elektronika MK-61")
    endif()
    qt_generate_deploy_app_script(TARGET elektronika-mk61
        OUTPUT_SCRIPT qt_deploy_script NO_TRANSLATIONS)
    install(SCRIPT "${qt_deploy_script}")
    # The CI source directory contains exact Qt module LICENSES and source links.
    set(MK61_QT_NOTICES_DIR "" CACHE PATH "Qt runtime license/provenance directory")
    if(NOT IS_DIRECTORY "${MK61_QT_NOTICES_DIR}")
        message(FATAL_ERROR "Set MK61_QT_NOTICES_DIR to the bundled Qt license/provenance directory")
    endif()
    if(APPLE)
        install(DIRECTORY "${MK61_QT_NOTICES_DIR}/"
            DESTINATION "elektronika-mk61.app/Contents/Resources/licenses/Qt")
    else()
        install(DIRECTORY "${MK61_QT_NOTICES_DIR}/"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/${PROJECT_NAME}/Qt")
    endif()
else()
    message(FATAL_ERROR "No desktop package format configured for ${CMAKE_SYSTEM_NAME}")
endif()

# Source archives are made from the reviewed Git commit in CI, never from an
# unfiltered working directory containing local research or credentials.
include(CPack)
configure_file("${CMAKE_CURRENT_LIST_DIR}/source-package-disabled.cmake"
    "${CPACK_SOURCE_OUTPUT_CONFIG_FILE}" COPYONLY)
