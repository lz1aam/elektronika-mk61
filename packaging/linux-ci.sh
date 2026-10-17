#!/bin/sh
# Invoked inside disposable build/runtime containers by packages.yml.
set -eu
mode=$1
family=$2
case "$family" in
    deb)
        apt-get update
        if [ "$mode" = build ]; then
            apt-get install -y --no-install-recommends cmake g++ make qt6-base-dev libqt6svg6 dpkg-dev file python3 desktop-file-utils
        else
            apt-get install -y --no-install-recommends python3 desktop-file-utils /source/build-package/packages/*.deb
        fi
        ;;
    rpm)
        if [ "$mode" = build ]; then
            dnf install -y cmake gcc-c++ make qt6-qtbase-devel qt6-qtsvg rpm-build python3 desktop-file-utils
        else
            dnf install -y python3 desktop-file-utils /source/build-package/packages/*.rpm
        fi
        ;;
    *) exit 2 ;;
esac
if [ "$mode" = build ]; then
    cmake -S . -B build-package -DCMAKE_BUILD_TYPE=Release \
        -DMK61_ENABLE_PACKAGING=ON -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_COMPILE_WARNING_AS_ERROR=ON -DBUILD_TESTING=ON
    cmake --build build-package --parallel 2
    ctest --test-dir build-package -C Release --output-on-failure --no-tests=error
    if [ "$family" = deb ]; then
        cpack --config build-package/CPackConfig.cmake -G DEB
    else
        cpack --config build-package/CPackConfig.cmake -G RPM
    fi
else
    python3 packaging/verify_install.py /usr
    desktop-file-validate /usr/share/applications/elektronika-mk61.desktop
    # Reinstall as an initial upgrade check, then exercise package removal.
    if [ "$family" = deb ]; then
        apt-get install -y --reinstall /source/build-package/packages/*.deb
        apt-get remove -y elektronika-mk61
    else
        dnf reinstall -y /source/build-package/packages/*.rpm
        dnf remove -y elektronika-mk61
    fi
    test ! -e /usr/bin/elektronika-mk61
    test ! -e /usr/share/applications/elektronika-mk61.desktop
    test ! -e /usr/share/icons/hicolor/scalable/apps/elektronika-mk61.svg
fi
