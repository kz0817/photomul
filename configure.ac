AC_PREREQ(2.63)

AC_INIT(photmul, 0.0.1)
AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-bzip2 tar-pax])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CXX

AC_CONFIG_MACRO_DIR([m4])
AM_SILENT_RULES([yes])

dnl **************************************************************
dnl Checks for GTK+
dnl **************************************************************
GTK_REQUIRED=3.6
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= $GTK_REQUIRED)

dnl **************************************************************
dnl Results
dnl **************************************************************

AC_OUTPUT([
Makefile
src/Makefile
])
