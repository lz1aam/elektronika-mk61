#!/usr/bin/env bash
set -euo pipefail

project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build_dir=${MK61_BUILD_DIR:-"${project_dir}/build-release"}
install_prefix=${1:-${MK61_INSTALL_PREFIX:-"${HOME:?}/.local"}}

if [[ ${install_prefix} != /* || ${install_prefix} == / ]]; then
    printf 'Installation prefix must be an absolute path other than /.\n' >&2
    exit 2
fi

cmake -S "${project_dir}" -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${install_prefix}"
cmake --build "${build_dir}" --parallel

if [[ ! -x ${build_dir}/elektronika-mk61 ]]; then
    printf 'Build did not produce elektronika-mk61; check that Qt 6 Widgets development files are installed.\n' >&2
    exit 1
fi

cmake --install "${build_dir}" --prefix "${install_prefix}"

printf 'Elektronika MK-61 Emulator was installed in %s\n' "${install_prefix}"
printf 'Executable: %s/bin/elektronika-mk61\n' "${install_prefix}"
