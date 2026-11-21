#!/usr/bin/env bash
# Refresh GIT_STATUS / COMPILATION_TIME baked into the binary, then rebuild.
# Shared by every project that has rbk at <project>/rbk.
#
# CMake (default): rewrite <build>/rbk/git_status_generated.h (buffer.cpp only)
# and rebuild rbk + the project targets via cmake --build (Ninja or Make). Do
# not re-run cmake configure — that would stamp flags.make and recompile every
# rbk translation unit. Do not call make in a Ninja tree: with no Makefile,
# GNU make treats the existing rbk/ dir and the binary as up to date.
#
# The project root is the git top level of the current directory, so run it
# from inside the project (not from inside rbk, which is its own repo).
#
# Usage:
#   ./rbk/gitTrick/bumpGitInlineInfo.sh                       # build/, targets: rbk <projectDirName>
#   ./rbk/gitTrick/bumpGitInlineInfo.sh build
#   ./rbk/gitTrick/bumpGitInlineInfo.sh build myTarget other  # targets: rbk myTarget other
#   ./rbk/gitTrick/bumpGitInlineInfo.sh build/Desktop_Qt_6_8_3-Debug   # qmake

set -euo pipefail

ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" || {
	echo "error: run this from inside the project git repo" >&2
	exit 1
}
BUILD_DIR="${1:-${ROOT}/build}"
shift || true
if [[ $# -gt 0 ]]; then
	TARGETS=("$@")
else
	TARGETS=("$(basename "${ROOT}")")
fi
BUFFER="${ROOT}/rbk/gitTrick/buffer.cpp"
WRITE_GIT_STATUS="${ROOT}/rbk/cmake/write_git_status.cmake"
JOBS="${CMAKE_BUILD_PARALLEL_LEVEL:-8}"  # per machine (laptop 8, desktop 24)

if [[ ! -f "${BUFFER}" ]]; then
	echo "error: missing ${BUFFER} (is ${ROOT} a project with rbk/?)" >&2
	exit 1
fi

if [[ ! -d "${BUILD_DIR}" ]]; then
	echo "error: build dir not found: ${BUILD_DIR}" >&2
	exit 1
fi
BUILD_DIR="$(cd "${BUILD_DIR}" && pwd)"

GIT_STATUS="$(git -C "${ROOT}" describe --always --dirty --abbrev=99)"
echo "git describe: ${GIT_STATUS}"
echo "project:      ${ROOT}"
echo "build dir:    ${BUILD_DIR}"

cd "${BUILD_DIR}"

if [[ -f CMakeCache.txt ]]; then
	HEADER="${BUILD_DIR}/rbk/git_status_generated.h"
	cmake -DSOURCE_DIR="${ROOT}" -DOUTPUT="${HEADER}" -P "${WRITE_GIT_STATUS}"
	# Force buffer.cpp even if the generated header is not yet in Make deps
	# (first bump after this change, or a generator that ignores OBJECT_DEPENDS).
	rm -f "${BUILD_DIR}/rbk/CMakeFiles/rbk.dir/gitTrick/buffer.cpp.o"
	TARGET_ARGS=(--target rbk)
	for t in "${TARGETS[@]}"; do
		TARGET_ARGS+=(--target "${t}")
	done
	echo "targets:      rbk ${TARGETS[*]}"
	cmake --build "${BUILD_DIR}" "${TARGET_ARGS[@]}" --parallel "${JOBS}"
elif [[ -f Makefile ]]; then
	# qmake: DEFINES use $(shell git describe ...) at compile time
	touch "${BUFFER}"
	make -j"${JOBS}"
else
	echo "error: neither CMakeCache.txt nor Makefile in ${BUILD_DIR}" >&2
	exit 1
fi

echo "done — restart the program to see:"
echo "  GIT_STATUS: ${GIT_STATUS}"
