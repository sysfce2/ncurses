dnl*****************************************************************************
dnl Copyright 1996,1997 by Thomas E. Dickey <dickey@clark.net>                 *
dnl All Rights Reserved.                                                       *
dnl                                                                            *
dnl Permission to use, copy, modify, and distribute this software and its      *
dnl documentation for any purpose and without fee is hereby granted, provided  *
dnl that the above copyright notice appear in all copies and that both that    *
dnl copyright notice and this permission notice appear in supporting           *
dnl documentation, and that the name of the above listed copyright holder(s)   *
dnl not be used in advertising or publicity pertaining to distribution of the  *
dnl software without specific, written prior permission. THE ABOVE LISTED      *
dnl COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,  *
dnl INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO     *
dnl EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY         *
dnl SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER       *
dnl RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF       *
dnl CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN        *
dnl CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.                   *
dnl*****************************************************************************
dnl $Id: aclocal.m4,v 1.79 1997/09/07 00:20:38 tom Exp $
dnl Macros used in NCURSES auto-configuration script.
dnl
dnl ---------------------------------------------------------------------------
dnl ---------------------------------------------------------------------------
dnl Construct the list of include-options for the C programs in the Ada95
dnl binding.
AC_DEFUN([CF_ADA_INCLUDE_DIRS],
[
ACPPFLAGS="$ACPPFLAGS -I. -I../../include"
if test "$srcdir" != "."; then
	ACPPFLAGS="$ACPPFLAGS -I\$(srcdir)/../../include"
fi
if test -z "$GCC"; then
	ACPPFLAGS="$ACPPFLAGS -I\$(includedir)"
elif test "$includedir" != "/usr/include"; then
	if test "$includedir" = '${prefix}/include' ; then
		if test $prefix != /usr ; then
			ACPPFLAGS="$ACPPFLAGS -I\$(includedir)"
		fi
	else
		ACPPFLAGS="$ACPPFLAGS -I\$(includedir)"
	fi
fi
AC_SUBST(ACPPFLAGS)
])dnl
dnl ---------------------------------------------------------------------------
dnl Test if 'bool' is a builtin type in the configured C++ compiler.  Some
dnl older compilers (e.g., gcc 2.5.8) don't support 'bool' directly; gcc
dnl 2.6.3 does, in anticipation of the ANSI C++ standard.
dnl
dnl Treat the configuration-variable specially here, since we're directly
dnl substituting its value (i.e., 1/0).
AC_DEFUN([CF_BOOL_DECL],
[
AC_MSG_CHECKING([for builtin c++ bool type])
AC_CACHE_VAL(cf_cv_builtin_bool,[
	AC_TRY_COMPILE([],[bool x = false],
		[cf_cv_builtin_bool=1],
		[cf_cv_builtin_bool=0])
	])
if test $cf_cv_builtin_bool = 1
then	AC_MSG_RESULT(yes)
else	AC_MSG_RESULT(no)
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl Test for the size of 'bool' in the configured C++ compiler (e.g., a type).
dnl Don't bother looking for bool.h, since it's been deprecated.
AC_DEFUN([CF_BOOL_SIZE],
[
AC_MSG_CHECKING([for size of c++ bool])
AC_CACHE_VAL(cf_cv_type_of_bool,[
	rm -f cf_test.out
	AC_TRY_RUN([
#include <stdlib.h>
#include <stdio.h>
#if HAVE_BUILTIN_H
#include <builtin.h>
#endif
main()
{
	FILE *fp = fopen("cf_test.out", "w");
	if (fp != 0) {
		bool x = false;
		if (sizeof(x) == sizeof(int))       fputs("int",  fp);
		else if (sizeof(x) == sizeof(char)) fputs("char", fp);
		else if (sizeof(x) == sizeof(short))fputs("short",fp);
		else if (sizeof(x) == sizeof(long)) fputs("long", fp);
		fclose(fp);
	}
	exit(0);
}
		],
		[cf_cv_type_of_bool=`cat cf_test.out`],
		[cf_cv_type_of_bool=unknown],
		[cf_cv_type_of_bool=unknown])
	])
	rm -f cf_test.out
AC_MSG_RESULT($cf_cv_type_of_bool)
if test $cf_cv_type_of_bool = unknown ; then
	AC_MSG_WARN(Assuming unsigned for type of bool)
	cf_cv_type_of_bool=unsigned
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl Determine the default configuration into which we'll install ncurses.  This
dnl can be overridden by the user's command-line options.  There's two items to
dnl look for:
dnl	1. the prefix (e.g., /usr)
dnl	2. the header files (e.g., /usr/include/ncurses)
dnl We'll look for a previous installation of ncurses and use the same defaults.
dnl
dnl We don't use AC_PREFIX_DEFAULT, because it gets evaluated too soon, and
dnl we don't use AC_PREFIX_PROGRAM, because we cannot distinguish ncurses's
dnl programs from a vendor's.
AC_DEFUN([CF_CFG_DEFAULTS],
[
AC_MSG_CHECKING(for prefix)
if test "x$prefix" = "xNONE" ; then
	case "$cf_cv_systype" in
		# non-vendor systems don't have a conflict
	OpenBSD|NetBSD|FreeBSD|Linux)	prefix=/usr
		;;
	*)	prefix=$ac_default_prefix
		;;
	esac
fi
AC_MSG_RESULT($prefix)
if test "x$prefix" = "xNONE" ; then
AC_MSG_CHECKING(for default include-directory)
test -n "$verbose" && echo 1>&6
for cf_symbol in \
	$includedir \
	$includedir/ncurses \
	$prefix/include \
	$prefix/include/ncurses \
	/usr/local/include \
	/usr/local/include/ncurses \
	/usr/include \
	/usr/include/ncurses
do
	cf_dir=`eval echo $cf_symbol`
	if test -f $cf_dir/curses.h ; then
	if ( fgrep NCURSES_VERSION $cf_dir/curses.h 2>&1 >/dev/null ) ; then
		includedir="$cf_symbol"
		test -n "$verbose"  && echo $ac_n "	found " 1>&6
		break
	fi
	fi
	test -n "$verbose"  && echo "	tested $cf_dir" 1>&6
done
AC_MSG_RESULT($includedir)
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl If we're trying to use g++, test if libg++ is installed (a rather common
dnl problem :-).  If we have the compiler but no library, we'll be able to
dnl configure, but won't be able to build the c++ demo program.
AC_DEFUN([CF_CXX_LIBRARY],
[
cf_cxx_library=unknown
if test $ac_cv_prog_gxx = yes; then
	AC_MSG_CHECKING([for libg++])
	cf_save="$LIBS"
	LIBS="$LIBS -lg++ -lm"
	AC_TRY_LINK([
#include <builtin.h>
	],
	[float foo=abs(1.0)],
	[cf_cxx_library=yes
	 CXXLIBS="$CXXLIBS -lg++ -lm"],
	[cf_cxx_library=no])
	LIBS="$cf_save"
	AC_MSG_RESULT($cf_cxx_library)
fi
])dnl
dnl ---------------------------------------------------------------------------
AC_DEFUN([CF_DIRS_TO_MAKE],
[
DIRS_TO_MAKE="lib"
for cf_item in $cf_list_models
do
	CF_OBJ_SUBDIR($cf_item,cf_subdir)
	DIRS_TO_MAKE="$DIRS_TO_MAKE $cf_subdir"
done
for cf_dir in $DIRS_TO_MAKE
do
	test ! -d $cf_dir && mkdir $cf_dir
done
AC_SUBST(DIRS_TO_MAKE)
])dnl
dnl ---------------------------------------------------------------------------
dnl Check if 'errno' is declared in <errno.h>
AC_DEFUN([CF_ERRNO],
[
AC_MSG_CHECKING([for errno external decl])
AC_CACHE_VAL(cf_cv_extern_errno,[
    AC_TRY_COMPILE([
#include <errno.h>],
        [int x = errno],
        [cf_cv_extern_errno=yes],
        [cf_cv_extern_errno=no])])
AC_MSG_RESULT($cf_cv_extern_errno)
test $cf_cv_extern_errno = no && AC_DEFINE(DECL_ERRNO)
])dnl
dnl ---------------------------------------------------------------------------
dnl Test for availability of useful gcc __attribute__ directives to quiet
dnl compiler warnings.  Though useful, not all are supported -- and contrary
dnl to documentation, unrecognized directives cause older compilers to barf.
AC_DEFUN([CF_GCC_ATTRIBUTES],
[
if test -n "$GCC"
then
cat > conftest.i <<EOF
#ifndef GCC_PRINTF
#define GCC_PRINTF 0
#endif
#ifndef GCC_SCANF
#define GCC_SCANF 0
#endif
#ifndef GCC_NORETURN
#define GCC_NORETURN /* nothing */
#endif
#ifndef GCC_UNUSED
#define GCC_UNUSED /* nothing */
#endif
EOF
if test -n "$GCC"
then
	AC_CHECKING([for gcc __attribute__ directives])
	changequote(,)dnl
cat > conftest.$ac_ext <<EOF
#line __oline__ "configure"
#include "confdefs.h"
#include "conftest.h"
#include "conftest.i"
#if	GCC_PRINTF
#define GCC_PRINTFLIKE(fmt,var) __attribute__((format(printf,fmt,var)))
#else
#define GCC_PRINTFLIKE(fmt,var) /*nothing*/
#endif
#if	GCC_SCANF
#define GCC_SCANFLIKE(fmt,var)  __attribute__((format(scanf,fmt,var)))
#else
#define GCC_SCANFLIKE(fmt,var)  /*nothing*/
#endif
extern void wow(char *,...) GCC_SCANFLIKE(1,2);
extern void oops(char *,...) GCC_PRINTFLIKE(1,2) GCC_NORETURN;
extern void foo(void) GCC_NORETURN;
int main(int argc GCC_UNUSED, char *argv[] GCC_UNUSED) { return 0; }
EOF
	changequote([,])dnl
	for cf_attribute in scanf printf unused noreturn
	do
		CF_UPPER(CF_ATTRIBUTE,$cf_attribute)
		cf_directive="__attribute__(($cf_attribute))"
		echo "checking for gcc $cf_directive" 1>&AC_FD_CC
		case $cf_attribute in
		scanf|printf)
		cat >conftest.h <<EOF
#define GCC_$CF_ATTRIBUTE 1
EOF
			;;
		*)
		cat >conftest.h <<EOF
#define GCC_$CF_ATTRIBUTE $cf_directive
EOF
			;;
		esac
		if AC_TRY_EVAL(ac_compile); then
			test -n "$verbose" && AC_MSG_RESULT(... $cf_attribute)
			cat conftest.h >>confdefs.h
#		else
#			sed -e 's/__attr.*/\/*nothing*\//' conftest.h >>confdefs.h
		fi
	done
else
	fgrep define conftest.i >>confdefs.h
fi
rm -rf conftest*
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl Check if the compiler supports useful warning options.  There's a few that
dnl we don't use, simply because they're too noisy:
dnl
dnl	-Wconversion (useful in older versions of gcc, but not in gcc 2.7.x)
dnl	-Wredundant-decls (system headers make this too noisy)
dnl	-Wtraditional (combines too many unrelated messages, only a few useful)
dnl
AC_DEFUN([CF_GCC_WARNINGS],
[cf_warn_CFLAGS=""
if test -n "$GCC"
then
	changequote(,)dnl
	cat > conftest.$ac_ext <<EOF
#line __oline__ "configure"
int main(int argc, char *argv[]) { return argv[argc-1] == 0; }
EOF
	changequote([,])dnl
	AC_CHECKING([for gcc warning options])
	cf_save_CFLAGS="$CFLAGS"
	cf_warn_CFLAGS="-W -Wall"
	cf_warn_CONST=""
	test "$with_ext_const" = yes && cf_warn_CONST="Wwrite-strings"
	for cf_opt in \
		Wbad-function-cast \
		Wcast-align \
		Wcast-qual \
		Winline \
		Wmissing-declarations \
		Wmissing-prototypes \
		Wnested-externs \
		Wpointer-arith \
		Wshadow \
		Wstrict-prototypes $cf_warn_CONST
	do
		CFLAGS="$cf_save_CFLAGS $cf_warn_CFLAGS -$cf_opt"
		if AC_TRY_EVAL(ac_compile); then
			test -n "$verbose" && AC_MSG_RESULT(... -$cf_opt)
			cf_warn_CFLAGS="$cf_warn_CFLAGS -$cf_opt"
		fi
	done
	rm -f conftest*
	CFLAGS="$cf_save_CFLAGS"
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl Verify Version of GNAT.
AC_DEFUN([CF_GNAT_VERSION],
[
changequote(<<, >>)dnl
cf_cv_gnat_version=`$cf_ada_make -v 2>&1 | grep '[0-9].[0-9][0-9]*' |\
  sed -e 's/[^0-9 \.]//g' | $AWK '{print $<<1>>;}'`
case $cf_cv_gnat_version in
  3.0[5-9]|3.[1-9]*|[4-9].*)
    ac_cv_prog_gnat_correct=yes
    ;;
  *) echo Unsupported GNAT version $cf_cv_gnat_version. Disabling Ada95 binding.
     ac_cv_prog_gnat_correct=no
     ;;
esac
changequote([, ])dnl
])
dnl ---------------------------------------------------------------------------
dnl Construct the list of include-options according to whether we're building
dnl in the source directory or using '--srcdir=DIR' option.  If we're building
dnl with gcc, don't append the includedir if it happens to be /usr/include,
dnl since that usually breaks gcc's shadow-includes.
AC_DEFUN([CF_INCLUDE_DIRS],
[
CPPFLAGS="$CPPFLAGS -I. -I../include"
if test "$srcdir" != "."; then
	CPPFLAGS="$CPPFLAGS -I\$(srcdir)/../include"
fi
if test -z "$GCC"; then
	CPPFLAGS="$CPPFLAGS -I\$(includedir)"
elif test "$includedir" != "/usr/include"; then
	if test "$includedir" = '${prefix}/include' ; then
		if test $prefix != /usr ; then
			CPPFLAGS="$CPPFLAGS -I\$(includedir)"
		fi
	else
		CPPFLAGS="$CPPFLAGS -I\$(includedir)"
	fi
fi
AC_SUBST(CPPFLAGS)
])dnl
dnl ---------------------------------------------------------------------------
dnl Append definitions and rules for the given models to the subdirectory
dnl Makefiles, and the recursion rule for the top-level Makefile.  If the
dnl subdirectory is a library-source directory, modify the LIBRARIES list in
dnl the corresponding makefile to list the models that we'll generate.
dnl
dnl For shared libraries, make a list of symbolic links to construct when
dnl generating each library.  The convention used for Linux is the simplest
dnl one:
dnl	lib<name>.so	->
dnl	lib<name>.so.<major>	->
dnl	lib<name>.so.<maj>.<minor>
AC_DEFUN([CF_LIB_RULES],
[
AC_REQUIRE([CF_SYSTYPE])
AC_REQUIRE([CF_SUBST_NCURSES_VERSION])
for cf_dir in $SRC_SUBDIRS
do
	if test -f $srcdir/$cf_dir/modules; then

		cf_libs_to_make=
		for cf_item in $CF_LIST_MODELS
		do
			CF_LIB_SUFFIX($cf_item,cf_suffix)
			cf_libs_to_make="$cf_libs_to_make ../lib/lib${cf_dir}${cf_suffix}"
		done

		sed -e "s@\@LIBS_TO_MAKE\@@$cf_libs_to_make@" \
			$cf_dir/Makefile >$cf_dir/Makefile.out
		mv $cf_dir/Makefile.out $cf_dir/Makefile

		$AWK -f $srcdir/mk-0th.awk \
			name=$cf_dir \
			$srcdir/$cf_dir/modules >>$cf_dir/Makefile

		for cf_item in $CF_LIST_MODELS
		do
			echo 'Appending rules for '$cf_item' model ('$cf_dir')'
			CF_UPPER(CF_ITEM,$cf_item)
			CF_LIB_SUFFIX($cf_item,cf_suffix)
			CF_OBJ_SUBDIR($cf_item,cf_subdir)

			# These dependencies really are for development, not
			# builds, but they are useful in porting, too.
			cf_depend="../include/ncurses_cfg.h"
			if test "$srcdir" = "."; then
				cf_reldir="."
			else
				cf_reldir="\$(srcdir)"
			fi
			if test -f $srcdir/$cf_dir/$cf_dir.priv.h; then
				cf_depend="$cf_depend $cf_reldir/$cf_dir.priv.h"
			elif test -f $srcdir/$cf_dir/curses.priv.h; then
				cf_depend="$cf_depend $cf_reldir/curses.priv.h"
			fi
			$AWK -f $srcdir/mk-1st.awk \
				name=$cf_dir \
				MODEL=$CF_ITEM \
				model=$cf_subdir \
				suffix=$cf_suffix \
				DoLinks=$cf_cv_do_symlinks \
				rmSoLocs=$cf_cv_rm_so_locs \
				overwrite=$WITH_OVERWRITE \
				depend="$cf_depend" \
				$srcdir/$cf_dir/modules >>$cf_dir/Makefile
			test $cf_dir = ncurses && WITH_OVERWRITE=no
			$AWK -f $srcdir/mk-2nd.awk \
				name=$cf_dir \
				MODEL=$CF_ITEM \
				model=$cf_subdir \
				srcdir=$srcdir \
				echo=$WITH_ECHO \
				$srcdir/$cf_dir/modules >>$cf_dir/Makefile
		done
	fi

	echo '	cd '$cf_dir' && $(MAKE) $(CF_MFLAGS) [$]@' >>Makefile
done

for cf_dir in $SRC_SUBDIRS
do
	if test -f $srcdir/$cf_dir/modules; then
		echo >> Makefile
		if test -f $srcdir/$cf_dir/headers; then
cat >> Makefile <<CF_EOF
install.includes \\
CF_EOF
		fi
if test "$cf_dir" != "c++" ; then
echo 'lint \' >> Makefile
fi
cat >> Makefile <<CF_EOF
lintlib \\
install.libs \\
install.$cf_dir ::
	cd $cf_dir && \$(MAKE) \$(CF_MFLAGS) \[$]@
CF_EOF
	elif test -f $srcdir/$cf_dir/headers; then
cat >> Makefile <<CF_EOF

install.libs \\
install.includes ::
	cd $cf_dir && \$(MAKE) \$(CF_MFLAGS) \[$]@
CF_EOF
fi
done

cat >> Makefile <<CF_EOF

install.data ::
	cd misc && \$(MAKE) \$(CF_MFLAGS) \[$]@

install.man ::
	cd man && \$(MAKE) \$(CF_MFLAGS) \[$]@

distclean ::
	rm -f config.cache config.log config.status Makefile include/ncurses_cfg.h
	rm -f headers.sh headers.sed
	rm -rf \$(DIRS_TO_MAKE)
CF_EOF

dnl If we're installing into a subdirectory of /usr/include, etc., we should
dnl prepend the subdirectory's name to the "#include" paths.  It won't hurt
dnl anything, and will make it more standardized.  It's awkward to decide this
dnl at configuration because of quoting, so we'll simply make all headers
dnl installed via a script that can do the right thing.

rm -f headers.sed headers.sh

dnl ( generating this script makes the makefiles a little tidier :-)
echo creating headers.sh
cat >headers.sh <<CF_EOF
#! /bin/sh
# This shell script is generated by the 'configure' script.  It is invoked in a
# subdirectory of the build tree.  It generates a sed-script in the parent
# directory that is used to adjust includes for header files that reside in a
# subdirectory of /usr/include, etc.
PRG=""
while test \[$]# != 3
do
PRG="\$PRG \[$]1"; shift
done
DST=\[$]1
REF=\[$]2
SRC=\[$]3
echo installing \$SRC in \$DST
case \$DST in
/*/include/*)
	TMP=\${TMPDIR-/tmp}/\`basename \$SRC\`
	if test ! -f ../headers.sed ; then
		END=\`basename \$DST\`
		for i in \`cat \$REF/../*/headers |fgrep -v "#"\`
		do
			NAME=\`basename \$i\`
			echo "s/<\$NAME>/<\$END\/\$NAME>/" >> ../headers.sed
		done
	fi
	rm -f \$TMP
	sed -f ../headers.sed \$SRC > \$TMP
	eval \$PRG \$TMP \$DST
	rm -f \$TMP
	;;
*)
	eval \$PRG \$SRC \$DST
	;;
esac
CF_EOF

chmod 0755 headers.sh

for cf_dir in $SRC_SUBDIRS
do
	if test -f $srcdir/$cf_dir/headers; then
	cat >>$cf_dir/Makefile <<CF_EOF
\$(INSTALL_PREFIX)\$(includedir) :
	\$(srcdir)/../mkinstalldirs \[$]@

install \\
install.libs \\
install.includes :: \$(INSTALL_PREFIX)\$(includedir) \\
CF_EOF
		j=""
		for i in `cat $srcdir/$cf_dir/headers |fgrep -v "#"`
		do
			test -n "$j" && echo "		$j \\" >>$cf_dir/Makefile
			j=$i
		done
		echo "		$j" >>$cf_dir/Makefile
		for i in `cat $srcdir/$cf_dir/headers |fgrep -v "#"`
		do
			echo "	@ ../headers.sh \$(INSTALL_DATA) \$(INSTALL_PREFIX)\$(includedir) \$(srcdir) $i" >>$cf_dir/Makefile
			test $i = curses.h && echo "	@ (cd \$(INSTALL_PREFIX)\$(includedir) && rm -f ncurses.h && \$(LN_S) curses.h ncurses.h)" >>$cf_dir/Makefile
		done
	fi
done

])dnl
dnl ---------------------------------------------------------------------------
dnl Compute the library-suffix from the given model name
AC_DEFUN([CF_LIB_SUFFIX],
[
	AC_REQUIRE([CF_SYSTYPE])
	AC_REQUIRE([CF_SUBST_NCURSES_VERSION])
	case $1 in
	normal)  $2='.a'   ;;
	debug)   $2='_g.a' ;;
	profile) $2='_p.a' ;;
	shared)
		case $cf_cv_systype in
		OpenBSD|NetBSD|FreeBSD)
			$2='.so.$(REL_VERSION)' ;;
		HP_UX)	$2='.sl'  ;;
		*)	$2='.so'  ;;
		esac
	esac
])dnl
dnl ---------------------------------------------------------------------------
dnl Compute the string to append to -library from the given model name
AC_DEFUN([CF_LIB_TYPE],
[
	case $1 in
	normal)  $2=''   ;;
	debug)   $2='_g' ;;
	profile) $2='_p' ;;
	shared)  $2=''   ;;
	esac
])dnl
dnl ---------------------------------------------------------------------------
dnl Some systems have a non-ANSI linker that doesn't pull in modules that have
dnl only data (i.e., no functions), for example NeXT.  On those systems we'll
dnl have to provide wrappers for global tables to ensure they're linked
dnl properly.
AC_DEFUN([CF_LINK_DATAONLY],
[
AC_MSG_CHECKING([if data-only library module links])
AC_CACHE_VAL(cf_cv_link_dataonly,[
	rm -f conftest.a
	changequote(,)dnl
	cat >conftest.$ac_ext <<EOF
#line __oline__ "configure"
int	testdata[3] = { 123, 456, 789 };
EOF
	changequote([,])dnl
	if AC_TRY_EVAL(ac_compile) ; then
		mv conftest.o data.o && \
		( $AR $AR_OPTS conftest.a data.o ) 2>&5 1>/dev/null
	fi
	rm -f conftest.$ac_ext data.o
	changequote(,)dnl
	cat >conftest.$ac_ext <<EOF
#line __oline__ "configure"
int	testfunc()
{
#if defined(NeXT)
	exit(1);	/* I'm told this linker is broken */
#else
	extern int testdata[3];
	return testdata[0] == 123
	   &&  testdata[1] == 456
	   &&  testdata[2] == 789;
#endif
}
EOF
	changequote([,])dnl
	if AC_TRY_EVAL(ac_compile); then
		mv conftest.o func.o && \
		( $AR $AR_OPTS conftest.a func.o ) 2>&5 1>/dev/null
	fi
	rm -f conftest.$ac_ext func.o
	( eval $ac_cv_prog_RANLIB conftest.a ) 2>&5 >/dev/null
	cf_saveLIBS="$LIBS"
	LIBS="conftest.a $LIBS"
	AC_TRY_RUN([
	int main()
	{
		extern int testfunc();
		exit (!testfunc());
	}
	],
	[cf_cv_link_dataonly=yes],
	[cf_cv_link_dataonly=no],
	[cf_cv_link_dataonly=unknown])
	LIBS="$cf_saveLIBS"
	])
AC_MSG_RESULT($cf_cv_link_dataonly)
test $cf_cv_link_dataonly = no && AC_DEFINE(BROKEN_LINKER)
])dnl
dnl ---------------------------------------------------------------------------
dnl Some 'make' programs support $(MAKEFLAGS), some $(MFLAGS), to pass 'make'
dnl options to lower-levels.  It's very useful for "make -n" -- if we have it.
dnl (GNU 'make' does both :-)
AC_DEFUN([CF_MAKEFLAGS],
[
AC_MSG_CHECKING([for makeflags variable])
AC_CACHE_VAL(cf_cv_makeflags,[
	cf_cv_makeflags=''
	for cf_option in '$(MFLAGS)' '-$(MAKEFLAGS)'
	do
		cat >cf_makeflags.tmp <<CF_EOF
all :
	echo '.$cf_option'
CF_EOF
		set cf_result=`${MAKE-make} -f cf_makeflags.tmp 2>/dev/null`
		if test "$cf_result" != "."
		then
			cf_cv_makeflags=$cf_option
			break
		fi
	done
	rm -f cf_makeflags.tmp])
AC_MSG_RESULT($cf_cv_makeflags)
AC_SUBST(cf_cv_makeflags)
])dnl
dnl ---------------------------------------------------------------------------
dnl Try to determine if the man-pages on the system are compressed, and if
dnl so, what format is used.  Use this information to construct a script that
dnl will install man-pages.
AC_DEFUN([CF_MAN_PAGES],
[AC_MSG_CHECKING(format of man-pages)
  if test -z "$MANPATH" ; then
    MANPATH="/usr/man:/usr/share/man"
  fi
  # look for the 'date' man-page (it's most likely to be installed!)
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:"
  cf_form=unknown
  for cf_dir in $MANPATH; do
    test -z "$cf_dir" && cf_dir=/usr/man
    cf_rename=""
    cf_format=no
changequote({{,}})dnl
    for cf_name in $cf_dir/*/date.[01]* $cf_dir/*/date
changequote([,])dnl
    do
       cf_test=`echo $cf_name | sed -e 's/*//'`
       if test "x$cf_test" = "x$cf_name" ; then
	  case "$cf_name" in
	  *.gz) cf_form=gzip;     cf_name=`basename $cf_name .gz`;;
	  *.Z)  cf_form=compress; cf_name=`basename $cf_name .Z`;;
	  *.0)	cf_form=BSDI; cf_format=yes;;
	  *)    cf_form=cat;;
	  esac
	  break
       fi
    done
    if test "$cf_form" != "unknown" ; then
       break
    fi
  done
  IFS="$ac_save_ifs"
  if test "$prefix" = "NONE" ; then
     cf_prefix="$ac_default_prefix"
  else
     cf_prefix="$prefix"
  fi

  # Debian 'man' program?
  test -f /etc/debian_version && \
  cf_rename=`cd $srcdir && pwd`/man/man_db.renames

  test ! -d man && mkdir man

  # Construct a sed-script to perform renaming within man-pages
  if test -n "$cf_rename" ; then
    fgrep -v \# $cf_rename | \
    sed -e 's/^/s\//' \
        -e 's/\./\\./' \
        -e 's/	/ /g' \
        -e 's/[ ]\+/\//' \
        -e s/\$/\\\/g/ >man/edit_man.sed
  fi
  if test $cf_format = yes ; then
    cf_subdir='$mandir/cat'
  else
    cf_subdir='$mandir/man'
  fi

cat >man/edit_man.sh <<CF_EOF
changequote({{,}})dnl
#! /bin/sh
# this script is generated by the configure-script
prefix="$cf_prefix"
datadir="$datadir"
MKDIRS="`cd $srcdir && pwd`/mkinstalldirs"
INSTALL="$INSTALL"
INSTALL_DATA="$INSTALL_DATA"
TMP=\${TMPDIR-/tmp}/man\$\$
trap "rm -f \$TMP" 0 1 2 5 15

mandir=\{{$}}1
shift

for i in \{{$}}*
do
case \$i in
*.[0-9]*)
	section=\`expr "\$i" : '.*\\.\\([0-9]\\)[xm]*'\`;
	if [ ! -d $cf_subdir\${section} ]; then
		\$MKDIRS $cf_subdir\$section
	fi
	source=\`basename \$i\`
CF_EOF
if test -z "$cf_rename" ; then
cat >>man/edit_man.sh <<CF_EOF
	target=$cf_subdir\${section}/\$source
	sed -e "s,@DATADIR@,\$datadir," < \$i >\$TMP
CF_EOF
else
cat >>man/edit_man.sh <<CF_EOF
	target=\`grep "^\$source" $cf_rename | $AWK '{print \{{$}}2}'\`
	if test -z "\$target" ; then
		echo '? missing rename for '\$source
		target="\$source"
	fi
	target="$cf_subdir\$section/\$target"
	sed -e 's,@DATADIR@,\$datadir,' < \$i | sed -f edit_man.sed >\$TMP
CF_EOF
fi
if test $cf_format = yes ; then
cat >>man/edit_man.sh <<CF_EOF
	nroff -man \$TMP >\$TMP.out
	mv \$TMP.out \$TMP
CF_EOF
fi
case "$cf_form" in
compress)
cat >>man/edit_man.sh <<CF_EOF
	if ( compress -f \$TMP )
	then
		mv \$TMP.Z \$TMP
		target="\$target.Z"
	fi
CF_EOF
  ;;
gzip)
cat >>man/edit_man.sh <<CF_EOF
	if ( gzip -f \$TMP )
	then
		mv \$TMP.gz \$TMP
		target="\$target.gz"
	fi
CF_EOF
  ;;
BSDI)
cat >>man/edit_man.sh <<CF_EOF
	# BSDI installs only .0 suffixes in the cat directories
	target="\`echo \$target|sed -e 's/\.[1-9]\+.\?/.0/'\`"
CF_EOF
  ;;
esac
cat >>man/edit_man.sh <<CF_EOF
	echo installing \$target
	\$INSTALL_DATA \$TMP \$target
	;;
esac
done 
CF_EOF
changequote([,])dnl
chmod 755 man/edit_man.sh
AC_MSG_RESULT($cf_form)
])dnl
dnl ---------------------------------------------------------------------------
dnl Compute the object-directory name from the given model name
AC_DEFUN([CF_OBJ_SUBDIR],
[
	case $1 in
	normal)  $2='objects' ;;
	debug)   $2='obj_g' ;;
	profile) $2='obj_p' ;;
	shared)  $2='obj_s' ;;
	esac
])dnl
dnl ---------------------------------------------------------------------------
dnl Force $INSTALL to be an absolute-path.  Otherwise, edit_man.sh and the
dnl misc/tabset install won't work properly.  Usually this happens only when
dnl using the fallback mkinstalldirs script
AC_DEFUN([CF_PROG_INSTALL],
[AC_PROG_INSTALL
case $INSTALL in
/*)
  ;;
*)
changequote({{,}})dnl
  cf_dir=`echo $INSTALL|sed -e 's%/[^/]*$%%'`
  test -z "$cf_dir" && cf_dir=.
changequote([,])dnl
  INSTALL=`cd $cf_dir && pwd`/`echo $INSTALL | sed -e 's:^.*/::'`
  ;;
esac
])dnl
dnl ---------------------------------------------------------------------------
dnl Attempt to determine if we've got one of the flavors of regular-expression
dnl code that we can support.
AC_DEFUN([CF_REGEX],
[
AC_MSG_CHECKING([for regular-expression headers])
AC_CACHE_VAL(cf_cv_regex,[
AC_TRY_LINK([#include <sys/types.h>
#include <regex.h>],[
	regex_t *p;
	int x = regcomp(p, "", 0);
	int y = regexec(p, "", 0, 0, 0);
	regfree(p);
	],[cf_cv_regex="regex.h"],[
	AC_TRY_LINK([#include <regexp.h>],[
		char *p = compile("", "", "", 0);
		int x = step("", "");
	],[cf_cv_regex="regexp.h"],[
		AC_TRY_LINK([#include <regexpr.h>],[
			char *p = compile("", "", "");
			int x = step("", "");
		],[cf_cv_regex="regexpr.h"])])])
])
AC_MSG_RESULT($cf_cv_regex)
case $cf_cv_regex in
	regex.h)   AC_DEFINE(HAVE_REGEX_H) ;;
	regexp.h)  AC_DEFINE(HAVE_REGEXP_H) ;;
	regexpr.h) AC_DEFINE(HAVE_REGEXPR_H) ;;
esac
])dnl
dnl ---------------------------------------------------------------------------
dnl Attempt to determine the appropriate CC/LD options for creating a shared
dnl library.
dnl
dnl Note: $(LOCAL_LDFLAGS) is used to link executables that will run within the 
dnl build-tree, i.e., by making use of the libraries that are compiled in ../lib
dnl We avoid compiling-in a ../lib path for the shared library since that can
dnl lead to unexpected results at runtime.
dnl $(LOCAL_LDFLAGS2) has the same intention but assumes that the shared libraries
dnl are compiled in ../../lib
dnl
dnl The variable 'cf_cv_do_symlinks' is used to control whether we configure
dnl to install symbolic links to the rel/abi versions of shared libraries.
dnl
dnl Some loaders leave 'so_locations' lying around.  It's nice to clean up.
AC_DEFUN([CF_SHARED_OPTS],
[
	AC_REQUIRE([CF_SYSTYPE])
	AC_REQUIRE([CF_SYSRELV])
	AC_REQUIRE([CF_SUBST_NCURSES_VERSION])
 	LOCAL_LDFLAGS=
 	LOCAL_LDFLAGS2=
	LD_SHARED_OPTS=

	cf_cv_do_symlinks=no
	cf_cv_rm_so_locs=no

	case $cf_cv_systype in
	HP_UX)
		# (tested with gcc 2.7.2 -- I don't have c89)
		if test "${CC}" = "gcc"; then
			CC_SHARED_OPTS='-fPIC'
			LD_SHARED_OPTS='-Xlinker +b -Xlinker $(libdir)'
		else
			CC_SHARED_OPTS='+Z'
			LD_SHARED_OPTS='+b $(libdir)'
		fi
		MK_SHARED_LIB='$(LD) -b -o $[@]'
		;;
	IRIX*)
		# tested with IRIX 5.2 and 'cc'.
		if test "${CC}" = "gcc"; then
			CC_SHARED_OPTS='-fPIC'
		else
			CC_SHARED_OPTS='-KPIC'
		fi
		MK_SHARED_LIB='$(LD) -shared -rdata_shared -soname `basename $[@]` -o $[@]'
		cf_cv_rm_so_locs=yes
		;;
	Linux)
		# tested with Linux 1.2.8 and gcc 2.7.0 (ELF)
		CC_SHARED_OPTS='-fPIC'
 		MK_SHARED_LIB='gcc -o $[@].$(REL_VERSION) -shared -Wl,-soname,`basename $[@].$(ABI_VERSION)`,-stats,-lc'
		test $cf_cv_ld_rpath = yes && MK_SHARED_LIB="$MK_SHARED_LIB -Wl,-rpath,\$(libdir)"
		if test $DFT_LWR_MODEL = "shared" ; then
 			LOCAL_LDFLAGS='-Wl,-rpath,../lib'
 			LOCAL_LDFLAGS2='-Wl,-rpath,../../lib'
		fi
		cf_cv_do_symlinks=yes
		;;
	OpenBSD|NetBSD|FreeBSD)
		CC_SHARED_OPTS='-fpic -DPIC'
		MK_SHARED_LIB='$(LD) -Bshareable -o $[@]'
		;;
	OSF1|MLS+)
		# tested with OSF/1 V3.2 and 'cc'
		# tested with OSF/1 V3.2 and gcc 2.6.3 (but the c++ demo didn't
		# link with shared libs).
		CC_SHARED_OPTS=''
 		MK_SHARED_LIB='$(LD) -o $[@].$(REL_VERSION) -set_version $(ABI_VERSION):$(REL_VERSION) -expect_unresolved "*" -shared -soname `basename $[@].$(ABI_VERSION)`'
		test $cf_cv_ld_rpath = yes && MK_SHARED_LIB="$MK_SHARED_LIB -rpath \$(libdir)"
		case $cf_cv_sysrelv in
		V4.*)
 			MK_SHARED_LIB="${MK_SHARED_LIB} -msym"
			;;
		esac
		if test $DFT_LWR_MODEL = "shared" ; then
 			LOCAL_LDFLAGS='-Wl,-rpath,../lib'
 			LOCAL_LDFLAGS2='-Wl,-rpath,../../lib'
		fi
		cf_cv_do_symlinks=yes
		cf_cv_rm_so_locs=yes
		;;
	SunOS)
		# tested with SunOS 4.1.1 and gcc 2.7.0
		# tested with SunOS 5.3 (solaris 2.3) and gcc 2.7.0
		if test $ac_cv_prog_gcc = yes; then
			CC_SHARED_OPTS='-fpic'
		else
			CC_SHARED_OPTS='-KPIC'
		fi
		case $cf_cv_sysrelv in
		4.*)
			MK_SHARED_LIB='$(LD) -assert pure-text -o $[@].$(REL_VERSION)'
			;;
		5.*)
			MK_SHARED_LIB='$(LD) -d y -G -h `basename $[@].$(ABI_VERSION)` -o $[@].$(REL_VERSION)'
			;;
		esac
		cf_cv_do_symlinks=yes
		;;
	UNIX_SV)
		# tested with UnixWare 1.1.2
		CC_SHARED_OPTS='-KPIC'
		MK_SHARED_LIB='$(LD) -d y -G -o $[@]'
		;;
	*)
		CC_SHARED_OPTS='unknown'
		MK_SHARED_LIB='echo unknown'
		;;
	esac
	AC_SUBST(CC_SHARED_OPTS)
	AC_SUBST(LD_SHARED_OPTS)
	AC_SUBST(MK_SHARED_LIB)
	AC_SUBST(LOCAL_LDFLAGS)
	AC_SUBST(LOCAL_LDFLAGS2)
])dnl
dnl ---------------------------------------------------------------------------
dnl Check for datatype 'speed_t', which is normally declared via either
dnl sys/types.h or termios.h
AC_DEFUN([CF_SPEED_TYPE],
[
AC_MSG_CHECKING([for speed_t])
AC_CACHE_VAL(cf_cv_type_speed_t,[
	AC_TRY_COMPILE([
#include <sys/types.h>
#if HAVE_TERMIOS_H
#include <termios.h>
#endif],
	[speed_t x = 0],
	[cf_cv_type_speed_t=yes],
	[cf_cv_type_speed_t=no])
	])
AC_MSG_RESULT($cf_cv_type_speed_t)
test $cf_cv_type_speed_t != yes && AC_DEFINE(speed_t,unsigned)
])dnl
dnl ---------------------------------------------------------------------------
dnl For each parameter, test if the source-directory exists, and if it contains
dnl a 'modules' file.  If so, add to the list $cf_cv_src_modules which we'll
dnl use in CF_LIB_RULES.
dnl
dnl This uses the configured value to make the lists SRC_SUBDIRS and
dnl SUB_MAKEFILES which are used in the makefile-generation scheme.
AC_DEFUN([CF_SRC_MODULES],
[
AC_MSG_CHECKING(for src modules)
TEST_DEPS="${LIB_PREFIX}${LIB_NAME}${DFT_DEP_SUFFIX}"
TEST_ARGS="-l${LIB_NAME}${DFT_ARG_SUFFIX}"
cf_cv_src_modules=
for cf_dir in $1
do
	if test -f $srcdir/$cf_dir/modules; then
		if test -z "$cf_cv_src_modules"; then
			cf_cv_src_modules=$cf_dir
		else
			cf_cv_src_modules="$cf_cv_src_modules $cf_dir"
		fi
		# Make the ncurses_cfg.h file record the library interface files as
		# well.  These are header files that are the same name as their
		# directory.  Ncurses is the only library that does not follow
		# that pattern.
		if test -f $srcdir/${cf_dir}/${cf_dir}.h; then
			CF_UPPER(cf_have_include,$cf_dir)
			AC_DEFINE_UNQUOTED(HAVE_${cf_have_include}_H)
			AC_DEFINE_UNQUOTED(HAVE_LIB${cf_have_include})
			TEST_DEPS="${LIB_PREFIX}${cf_dir}${DFT_DEP_SUFFIX} $TEST_DEPS"
			TEST_ARGS="-l${cf_dir}${DFT_ARG_SUFFIX} $TEST_ARGS"
		fi
	fi
done
AC_MSG_RESULT($cf_cv_src_modules)
TEST_ARGS="-L${LIB_DIR} $TEST_ARGS"
AC_SUBST(TEST_DEPS)
AC_SUBST(TEST_ARGS)

SRC_SUBDIRS="man include"
for cf_dir in $cf_cv_src_modules
do
	SRC_SUBDIRS="$SRC_SUBDIRS $cf_dir"
done
SRC_SUBDIRS="$SRC_SUBDIRS misc test"
test $cf_cxx_library != no && SRC_SUBDIRS="$SRC_SUBDIRS c++"

ADA_SUBDIRS=
if test "$ac_cv_prog_gnat_correct" = yes && test -d $srcdir/Ada95; then
   SRC_SUBDIRS="$SRC_SUBDIRS Ada95"
   ADA_SUBDIRS="gen ada_include samples"
fi

SUB_MAKEFILES=
for cf_dir in $SRC_SUBDIRS
do
	SUB_MAKEFILES="$SUB_MAKEFILES $cf_dir/Makefile"
done

if test -n "$ADA_SUBDIRS"; then
   for cf_dir in $ADA_SUBDIRS
   do	
      SUB_MAKEFILES="$SUB_MAKEFILES Ada95/$cf_dir/Makefile"
   done
   AC_SUBST(ADA_SUBDIRS)
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl	Remove "-g" option from the compiler options
AC_DEFUN([CF_STRIP_G_OPT],
[$1=`echo ${$1} | sed -e 's/-g //' -e 's/-g$//'`])dnl
dnl ---------------------------------------------------------------------------
dnl	Shorthand macro for substituting things that the user may override
dnl	with an environment variable.
dnl
dnl	$1 = long/descriptive name
dnl	$2 = environment variable
dnl	$3 = default value
AC_DEFUN([CF_SUBST],
[AC_CACHE_VAL(cf_cv_subst_$2,[
AC_MSG_CHECKING(for $1 (symbol $2))
test -z "[$]$2" && $2=$3
AC_MSG_RESULT([$]$2)
AC_SUBST($2)
cf_cv_subst_$2=[$]$2])
$2=${cf_cv_subst_$2}
])dnl
dnl ---------------------------------------------------------------------------
dnl Get the version-number for use in shared-library naming, etc.
AC_DEFUN([CF_SUBST_NCURSES_VERSION],
[
changequote(,)dnl
NCURSES_MAJOR="`egrep '^NCURSES_MAJOR[ 	]*=' $srcdir/dist.mk | sed -e 's/^[^0-9]*//'`"
NCURSES_MINOR="`egrep '^NCURSES_MINOR[ 	]*=' $srcdir/dist.mk | sed -e 's/^[^0-9]*//'`"
NCURSES_PATCH="`egrep '^NCURSES_PATCH[ 	]*=' $srcdir/dist.mk | sed -e 's/^[^0-9]*//'`"
changequote([,])dnl
cf_cv_abi_version=${NCURSES_MAJOR}
cf_cv_rel_version=${NCURSES_MAJOR}.${NCURSES_MINOR}
dnl Show the computed version, for logging
AC_MSG_RESULT(Configuring NCURSES $cf_cv_rel_version ABI $cf_cv_abi_version (`date`))
dnl We need these values in the generated headers
AC_SUBST(NCURSES_MAJOR)
AC_SUBST(NCURSES_MINOR)
AC_SUBST(NCURSES_PATCH)
dnl We need these values in the generated makefiles
AC_SUBST(cf_cv_rel_version)
AC_SUBST(cf_cv_abi_version)
AC_SUBST(cf_cv_builtin_bool)
AC_SUBST(cf_cv_type_of_bool)
])dnl
dnl ---------------------------------------------------------------------------
dnl Derive the system-release (our secondary clue to the method of building
dnl shared libraries).
AC_DEFUN([CF_SYSRELV],
[
AC_MSG_CHECKING(for system release version)
AC_CACHE_VAL(cf_cv_sysrelv,[
AC_ARG_WITH(system-release,
[  --with-system-relv=XXX  test: override derived host system-release version],
[cf_cv_sysrelv=$withval],
[
changequote(,)dnl
cf_cv_sysrelv="`(uname -r || echo unknown) 2>/dev/null |sed -e s'/[:\/-]/_/'g  | sed 1q`"
changequote([,])dnl
if test -z "$cf_cv_sysrelv"; then cf_cv_sysrelv=unknown;fi
])])
AC_MSG_RESULT($cf_cv_sysrelv)
])dnl
dnl ---------------------------------------------------------------------------
dnl Derive the system-type (our main clue to the method of building shared
dnl libraries).
AC_DEFUN([CF_SYSTYPE],
[
AC_MSG_CHECKING(for system type)
AC_CACHE_VAL(cf_cv_systype,[
AC_ARG_WITH(system-type,
[  --with-system-type=XXX  test: override derived host system-type],
[cf_cv_systype=$withval],
[
changequote(,)dnl
cf_cv_systype="`(uname -s || hostname || echo unknown) 2>/dev/null |sed -e s'/[:\/.-]/_/'g  | sed 1q`"
changequote([,])dnl
if test -z "$cf_cv_systype"; then cf_cv_systype=unknown;fi
])])
AC_MSG_RESULT($cf_cv_systype)
])dnl
dnl ---------------------------------------------------------------------------
dnl On some systems ioctl(fd, TIOCGWINSZ, &size) will always return {0,0} until
dnl ioctl(fd, TIOCSWINSZ, &size) is called to explicitly set the size of the
dnl screen.
dnl
dnl Attempt to determine if we're on such a system by running a test-program.
dnl This won't work, of course, if the configure script is run in batch mode,
dnl since we've got to have access to the terminal.
dnl
dnl 1996/4/26 - Converted this into a simple test for able-to-compile, since
dnl we're reminded that _nc_get_screensize() does the same functional test.
AC_DEFUN([CF_TIOCGWINSZ],
[
AC_MSG_CHECKING([for working TIOCGWINSZ])
AC_CACHE_VAL(cf_cv_use_tiocgwinsz,[
	AC_TRY_RUN([
#if HAVE_TERMIOS_H
#include <termios.h>
#endif
#if SYSTEM_LOOKS_LIKE_SCO
/* they neglected to define struct winsize in termios.h -- it's only
   in termio.h	*/
#include	<sys/stream.h>
#include	<sys/ptem.h>
#endif
#if !defined(sun) || !defined(HAVE_TERMIOS_H)
#include <sys/ioctl.h>
#endif
int main()
{
	static	struct winsize size;
	int fd;
	for (fd = 0; fd <= 2; fd++) {	/* try in/out/err in case redirected */
		if (ioctl(0, TIOCGWINSZ, &size) == 0
		 && size.ws_row > 0
		 && size.ws_col > 0)
			exit(0);
	}
	exit(0);	/* in either case, it compiles & links ... */
}
		],
		[cf_cv_use_tiocgwinsz=yes],
		[cf_cv_use_tiocgwinsz=no],
		[cf_cv_use_tiocgwinsz=unknown])
	])
AC_MSG_RESULT($cf_cv_use_tiocgwinsz)
test $cf_cv_use_tiocgwinsz != yes && AC_DEFINE(BROKEN_TIOCGWINSZ)
])dnl
dnl ---------------------------------------------------------------------------
dnl Determine the type we should use for chtype (and attr_t, which is treated
dnl as the same thing).  We want around 32 bits, so on most machines want a
dnl long, but on newer 64-bit machines, probably want an int.  If we're using
dnl wide characters, we have to have a type compatible with that, as well.
AC_DEFUN([CF_TYPEOF_CHTYPE],
[
AC_MSG_CHECKING([for type of chtype])
AC_CACHE_VAL(cf_cv_typeof_chtype,[
		AC_TRY_RUN([
#if USE_WIDEC_SUPPORT
#include <stddef.h>	/* we want wchar_t */
#define WANT_BITS 39
#else
#define WANT_BITS 31
#endif
#include <stdio.h>
int main()
{
	FILE *fp = fopen("cf_test.out", "w");
	if (fp != 0) {
		char *result = "long";
#if USE_WIDEC_SUPPORT
		/*
		 * If wchar_t is smaller than a long, it must be an int or a
		 * short.  We prefer not to use a short anyway.
		 */
		if (sizeof(unsigned long) > sizeof(wchar_t))
			result = "int";
#endif
		if (sizeof(unsigned long) > sizeof(unsigned int)) {
			int n;
			unsigned int x;
			for (n = 0; n < WANT_BITS; n++) {
				unsigned int y = (x >> n);
				if (y != 1 || x == 0) {
					x = 0;
					break;
				}
			}
			/*
			 * If x is nonzero, an int is big enough for the bits
			 * that we want.
			 */
			result = (x != 0) ? "int" : "long";
		}
		fputs(result, fp);
		fclose(fp);
	}
	exit(0);
}
		],
		[cf_cv_typeof_chtype=`cat cf_test.out`],
		[cf_cv_typeof_chtype=long],
		[cf_cv_typeof_chtype=long])
		rm -f cf_test.out
	])
AC_MSG_RESULT($cf_cv_typeof_chtype)
AC_SUBST(cf_cv_typeof_chtype)
AC_DEFINE_UNQUOTED(TYPEOF_CHTYPE,$cf_cv_typeof_chtype)
])dnl
dnl ---------------------------------------------------------------------------
dnl
AC_DEFUN([CF_TYPE_SIGACTION],
[
AC_MSG_CHECKING([for type sigaction_t])
AC_CACHE_VAL(cf_cv_type_sigaction,[
	AC_TRY_COMPILE([
#include <signal.h>],
		[sigaction_t x],
		[cf_cv_type_sigaction=yes],
		[cf_cv_type_sigaction=no])])
AC_MSG_RESULT($cf_cv_type_sigaction)
test $cf_cv_type_sigaction = yes && AC_DEFINE(HAVE_TYPE_SIGACTION)
])dnl
dnl ---------------------------------------------------------------------------
dnl Make an uppercase version of a variable
dnl $1=uppercase($2)
AC_DEFUN([CF_UPPER],
[
changequote(,)dnl
$1=`echo $2 | tr '[a-z]' '[A-Z]'`
changequote([,])dnl
])dnl
dnl ---------------------------------------------------------------------------
dnl Compute the shift-mask that we'll use for wide-character indices.  We use
dnl all but the index portion of chtype for storing attributes.
AC_DEFUN([CF_WIDEC_SHIFT],
[
AC_REQUIRE([CF_TYPEOF_CHTYPE])
AC_MSG_CHECKING([for number of bits in chtype])
AC_CACHE_VAL(cf_cv_shift_limit,[
	AC_TRY_RUN([
#include <stdio.h>
int main()
{
	FILE *fp = fopen("cf_test.out", "w");
	if (fp != 0) {
		int n;
		unsigned TYPEOF_CHTYPE x = 1L;
		for (n = 0; ; n++) {
			unsigned long y = (x >> n);
			if (y != 1 || x == 0)
				break;
			x <<= 1;
		}
		fprintf(fp, "%d", n);
		fclose(fp);
	}
	exit(0);
}
		],
		[cf_cv_shift_limit=`cat cf_test.out`],
		[cf_cv_shift_limit=32],
		[cf_cv_shift_limit=32])
		rm -f cf_test.out
	])
AC_MSG_RESULT($cf_cv_shift_limit)
AC_SUBST(cf_cv_shift_limit)

AC_MSG_CHECKING([for width of character-index])
AC_CACHE_VAL(cf_cv_widec_shift,[
if test ".$with_widec" = ".yes" ; then
	cf_attrs_width=39
	if ( expr $cf_cv_shift_limit \> $cf_attrs_width >/dev/null )
	then
		cf_cv_widec_shift=`expr 16 + $cf_cv_shift_limit - $cf_attrs_width`
	else
		cf_cv_widec_shift=16
	fi
else
	cf_cv_widec_shift=8
fi
])
AC_MSG_RESULT($cf_cv_widec_shift)
AC_SUBST(cf_cv_widec_shift)
])dnl
