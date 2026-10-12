#!/usr/bin/env bash
set -euo pipefail

install_prefix=${1:-${MK61_INSTALL_PREFIX:-"${HOME:?}/.local"}}

if [[ ${install_prefix} != /* || ${install_prefix} == / ]]; then
    printf 'Installation prefix must be an absolute path other than /.\n' >&2
    exit 2
fi

installed_files=(
    "${install_prefix}/bin/elektronika-mk61"
    "${install_prefix}/share/applications/elektronika-mk61.desktop"
    "${install_prefix}/share/icons/hicolor/scalable/apps/elektronika-mk61.svg"
    "${install_prefix}/share/doc/elektronika-mk61/LICENSE"
    "${install_prefix}/share/doc/elektronika-mk61/NOTICE"
)

removed=0
for installed_file in "${installed_files[@]}"; do
    if [[ -e ${installed_file} || -L ${installed_file} ]]; then
        rm -- "${installed_file}"
        printf 'Removed %s\n' "${installed_file}"
        removed=1
    fi
done

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "${install_prefix}/share/applications" \
        >/dev/null 2>&1 || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache --force --ignore-theme-index \
        "${install_prefix}/share/icons/hicolor" >/dev/null 2>&1 || true
fi

if (( removed == 0 )); then
    printf 'Elektronika MK-61 Emulator is not installed in %s\n' \
        "${install_prefix}"
else
    printf 'Elektronika MK-61 Emulator was removed from %s\n' \
        "${install_prefix}"
fi
