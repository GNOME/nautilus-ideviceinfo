#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(nautilus-ideviceinfo, 1.0.0, nospam@nowhere.com)
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

LIBIMOBILE_REQS=0.9.7
GLIB2_REQS=2.14.1
LIBPLIST_REQS=0.15
GTK2_REQS=2.16
NAUTILUS_REQS=2.21.2
LIBGPOD_REQS=0.7.90

# Checks for libraries.
PKG_CHECK_MODULES(WIDGET, libimobiledevice-1.0 >= $LIBIMOBILE_REQS
		          glib-2.0 >= $GLIB2_REQS
			  libplist >= $LIBPLIST_REQS
			  gtk+-2.0 >= $GTK2_REQS)
PKG_CHECK_MODULES(libnautilusextension, libnautilus-extension >= $NAUTILUS_REQS)
PKG_CHECK_MODULES(libgpod, libgpod-1.0 >= $LIBGPOD_REQS, enable_libgpod=yes, enable_libgpod=no)

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([strcasecmp strdup strerror strndup])

if test "x$enable_libgpod" = "xyes"; then
	AC_DEFINE(HAVE_LIBGPOD, 1, [Define if you have libgpod support])
	AC_SUBST(libgpod_CFLAGS)
	AC_SUBST(libgpod_LIBS)
fi
AM_CONDITIONAL(HAVE_LIBGPOD, test x"$enable_libgpod" = xyes)

NAUTILUS_EXTENSION_DIR=`$PKG_CONFIG --variable=extensiondir libnautilus-extension`
AC_SUBST(NAUTILUS_EXTENSION_DIR)

AS_COMPILER_FLAGS(GLOBAL_CFLAGS, "-Wall -Wextra -Wmissing-declarations -Wredundant-decls -Wshadow -Wpointer-arith  -Wwrite-strings -Wswitch-default -Wno-unused-parameter")
AC_SUBST(GLOBAL_CFLAGS)

# i18n
GETTEXT_PACKAGE=nautilus-ideviceinfo
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], "$GETTEXT_PACKAGE", [Gettext package])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])

echo "
Configuration for $PACKAGE $VERSION:
-------------------------------------------

  Installation directory: $NAUTILUS_EXTENSION_DIR

  Now type 'make' to build $PACKAGE $VERSION,
  and then 'make install' for installation.
"
