# Writes a header holding the current git describe, for gitTrick/buffer.cpp only.
#
# Run in script mode (cmake -P) both at configure time and as a PRE_BUILD step:
#   cmake -DSOURCE_DIR=<repo> -DOUTPUT=<path/git_status_generated.h> -P write_git_status.cmake
#
# The header is deliberately not a target-wide -DGIT_STATUS=..., which would live in
# flags.make and rebuild every rbk TU on any reconfigure.

if(NOT DEFINED SOURCE_DIR)
	message(FATAL_ERROR "write_git_status.cmake: SOURCE_DIR is required")
endif()
if(NOT DEFINED OUTPUT)
	message(FATAL_ERROR "write_git_status.cmake: OUTPUT is required")
endif()

execute_process(
	COMMAND git -C "${SOURCE_DIR}" describe --always --dirty --abbrev=99
	OUTPUT_VARIABLE _git_status
	OUTPUT_STRIP_TRAILING_WHITESPACE
	ERROR_QUIET
)

# No git, no repo, no tags: buffer.cpp guards on #ifdef GIT_STATUS and says
# "Not Available", so an empty define is better than a wrong one.
set(_content "#ifndef GIT_STATUS_GENERATED_H\n#define GIT_STATUS_GENERATED_H\n")
if(_git_status)
	string(REPLACE "\\" "\\\\" _git_status "${_git_status}")
	string(REPLACE "\"" "\\\"" _git_status "${_git_status}")
	string(APPEND _content "#define GIT_STATUS \"${_git_status}\"\n")
endif()
string(APPEND _content "#endif // GIT_STATUS_GENERATED_H\n")

# Rewriting unconditionally would bump the mtime on every build and recompile
# buffer.cpp every time, which is exactly what this header exists to avoid.
set(_previous "")
if(EXISTS "${OUTPUT}")
	file(READ "${OUTPUT}" _previous)
endif()

if(NOT _previous STREQUAL _content)
	file(WRITE "${OUTPUT}" "${_content}")
endif()
