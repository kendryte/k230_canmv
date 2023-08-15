set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
set(KCONFIG_ROOT ${CMAKE_SOURCE_DIR}/Kconfig)
set(BOARD_DIR ${CMAKE_SOURCE_DIR}/configs)
set(AUTOCONF_DIR ${CMAKE_CURRENT_BINARY_DIR}/kconfig/include/generated)
set(AUTOCONF_H ${AUTOCONF_DIR}/autoconf.h)

# Re-configure (Re-execute all CMakeLists.txt code) when autoconf.h changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AUTOCONF_H})

include(cmake/extensions.cmake)
include(cmake/python.cmake)
include(cmake/kconfig.cmake)
