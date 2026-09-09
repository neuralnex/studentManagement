# Locate libpqxx (PostgreSQL C++ binding) on systems where the Debian
# libpqxx-dev package ships headers and libs but no CMake config file.
#
# Defines:
#   pqxx_FOUND
#   pqxx_INCLUDE_DIRS
#   pqxx_LIBRARIES
#   pqxx_VERSION
#   pqxx::pqxx  - IMPORTED INTERFACE target

find_path(
    pqxx_INCLUDE_DIR
    NAMES pqxx/pqxx
    HINTS /usr/include /usr/local/include
)

find_library(
    pqxx_LIBRARY
    NAMES pqxx
    HINTS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
)

# Best-effort version: pqxx 8.x exposes PQXX_VERSION macro.
if(EXISTS "${pqxx_INCLUDE_DIR}/pqxx/version.hxx")
    file(STRINGS "${pqxx_INCLUDE_DIR}/pqxx/version.hxx" _pqxx_version_line
         REGEX "^#define[\t ]+PQXX_VERSION[\t ]+\".*\"")
    if(_pqxx_version_line)
        string(REGEX REPLACE
               "^#define[\t ]+PQXX_VERSION[\t ]+\"([^\"]+)\".*"
               "\\1" pqxx_VERSION "${_pqxx_version_line}")
    endif()
endif()

if(NOT pqxx_VERSION)
    set(pqxx_VERSION "8.0")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pqxx
    REQUIRED_VARS pqxx_INCLUDE_DIR pqxx_LIBRARY
    VERSION_VAR   pqxx_VERSION
)

if(pqxx_FOUND AND NOT TARGET pqxx::pqxx)
    add_library(pqxx::pqxx INTERFACE IMPORTED)
    set_target_properties(pqxx::pqxx PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${pqxx_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${pqxx_LIBRARY}"
    )
endif()

if(pqxx_FOUND)
    set(pqxx_INCLUDE_DIRS "${pqxx_INCLUDE_DIR}")
    set(pqxx_LIBRARIES    "${pqxx_LIBRARY}")
endif()

mark_as_advanced(pqxx_INCLUDE_DIR pqxx_LIBRARY pqxx_VERSION)
