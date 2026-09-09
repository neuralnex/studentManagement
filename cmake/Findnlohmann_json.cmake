# Locate nlohmann_json (header-only) on Debian/Ubuntu systems where the
# nlohmann-json3-dev package ships headers but no CMake config file.
#
# Defines:
#   nlohmann_json_FOUND         - True if found
#   nlohmann_json_INCLUDE_DIRS  - Path to headers containing <nlohmann/json.hpp>
#   nlohmann_json_VERSION       - Reported library version (best effort)
#   nlohmann_json::nlohmann_json - IMPORTED INTERFACE target

find_path(
    nlohmann_json_INCLUDE_DIR
    NAMES nlohmann/json.hpp
    HINTS /usr/include /usr/local/include
)

# Best-effort version detection. Prefer a header that exposes the version,
# then fall back to the package name.
set(_nlohmann_json_HINTS
    "${nlohmann_json_INCLUDE_DIR}/nlohmann/detail/macro_scope.hpp"
    "${nlohmann_json_INCLUDE_DIR}/nlohmann/json.hpp"
)
foreach(_hint ${_nlohmann_json_HINTS})
    if(EXISTS "${_hint}")
        file(STRINGS "${_hint}" _nlohmann_version_line
             REGEX "^#define[\t ]+NLOHMANN_JSON_VERSION[\t ]+\".*\"")
        if(_nlohmann_version_line)
            string(REGEX REPLACE
                   "^#define[\t ]+NLOHMANN_JSON_VERSION[\t ]+\"([^\"]+)\".*"
                   "\\1" nlohmann_json_VERSION "${_nlohmann_version_line}")
            break()
        endif()
    endif()
endforeach()

if(NOT nlohmann_json_VERSION)
    set(nlohmann_json_VERSION "3.11.3")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(nlohmann_json
    REQUIRED_VARS nlohmann_json_INCLUDE_DIR
    VERSION_VAR   nlohmann_json_VERSION
)

if(nlohmann_json_FOUND AND NOT TARGET nlohmann_json::nlohmann_json)
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${nlohmann_json_INCLUDE_DIR}"
    )
endif()

if(nlohmann_json_FOUND)
    set(nlohmann_json_INCLUDE_DIRS "${nlohmann_json_INCLUDE_DIR}")
endif()

mark_as_advanced(nlohmann_json_INCLUDE_DIR nlohmann_json_VERSION)
