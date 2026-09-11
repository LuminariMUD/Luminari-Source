# Cross toolchain for aarch64-linux-gnu. Used by the cross-aarch64 preset.
#
# Requires the GNU cross compiler (Debian/Ubuntu: gcc-aarch64-linux-gnu) and a
# sysroot that already contains the aarch64 development packages listed in
# docs/development/CMAKE_BUILD_GUIDE.md. Point LUMINARI_SYSROOT at that root:
#
#   cmake --preset cross-aarch64 -DLUMINARI_SYSROOT=/srv/sysroots/aarch64
#
# pkg-config is redirected into the sysroot so the pkg_check_modules() calls in
# CMakeLists.txt resolve libevent, json-c, gd, and the MariaDB client there.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(LUMINARI_CROSS_TRIPLE aarch64-linux-gnu)
set(CMAKE_C_COMPILER ${LUMINARI_CROSS_TRIPLE}-gcc)

set(LUMINARI_SYSROOT "" CACHE PATH "Sysroot holding the aarch64 development packages")
if (LUMINARI_SYSROOT)
    set(CMAKE_SYSROOT ${LUMINARI_SYSROOT})
    set(CMAKE_FIND_ROOT_PATH ${LUMINARI_SYSROOT})
    set(ENV{PKG_CONFIG_SYSROOT_DIR} ${LUMINARI_SYSROOT})
    set(ENV{PKG_CONFIG_LIBDIR}
        "${LUMINARI_SYSROOT}/usr/lib/${LUMINARI_CROSS_TRIPLE}/pkgconfig:${LUMINARI_SYSROOT}/usr/share/pkgconfig")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
