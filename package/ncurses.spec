Summary: shared libraries for terminal handling
Name: ncurses6
Version: 6.1
Release: 20190928
License: X11
Group: Development/Libraries
Source: ncurses-%{version}-%{release}.tgz
# URL: https://invisible-island.net/ncurses/

%define CC_NORMAL -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wconversion
%define CC_STRICT %{CC_NORMAL} -W -Wbad-function-cast -Wcast-align -Wcast-qual -Wmissing-declarations -Wnested-externs -Wpointer-arith -Wwrite-strings -ansi -pedantic

%global MY_ABI 6

# save value before redefining
%global sys_libdir %{_libdir}

# was redefined...
#global _prefix /usr/local/ncurses#{MY_ABI}

%global MY_PKG %{sys_libdir}/pkgconfig
%define MYDATA /usr/local/ncurses/share/terminfo

%description
The ncurses library routines are a terminal-independent method of
updating character screens with reasonable optimization.

This package is used for testing ABI %{MY_ABI}.

%prep

%global is_mandriva %(test -f /etc/mandriva-release && echo 1 || echo 0)
%global is_redhat   %(test -f /etc/redhat-release && echo 1 || echo 0)
%global is_suse     %(test -f /etc/SuSE-release && echo 1 || echo 0)

# nor are debug-symbols
%define debug_package %{nil}

%if %{is_mandriva}
%define _disable_ld_as_needed 1
%define _disable_ld_no_undefined 1
# libtool is not used here...
%define _disable_libtoolize 1
%define _disable_ld_build_id 1
%endif

%if %{is_redhat}
# workaround for toolset breakage in Fedora 28
%define _test_relink --enable-relink
%else
%define _test_relink --disable-relink
%endif

%setup -q -n ncurses-%{version}-%{release}

%build
%define CFG_OPTS \\\
	--target %{_target_platform} \\\
	--prefix=%{_prefix} \\\
	--bindir=%{_bindir} \\\
	--includedir=%{_includedir} \\\
	--libdir=%{_libdir} \\\
	--includedir='${prefix}/include' \\\
	--disable-echo \\\
	--disable-getcap \\\
	--disable-leaks \\\
	--disable-macros  \\\
	--disable-overwrite  \\\
	%{_test_relink}  \\\
	--disable-termcap \\\
	--enable-hard-tabs \\\
	--enable-opaque-curses \\\
	--enable-opaque-form \\\
	--enable-opaque-menu \\\
	--enable-opaque-panel \\\
	--enable-pc-files \\\
	--enable-rpath \\\
	--enable-warnings \\\
	--enable-wgetch-events \\\
	--enable-widec \\\
	--enable-xmc-glitch \\\
	--program-suffix=%{MY_ABI} \\\
	--verbose \\\
	--with-abi-version=%{MY_ABI} \\\
	--with-config-suffix=dev \\\
	--with-cxx-shared \\\
	--with-default-terminfo-dir=%{MYDATA} \\\
	--with-develop \\\
	--with-extra-suffix=%{MY_ABI} \\\
	--with-install-prefix=$RPM_BUILD_ROOT \\\
	--with-pkg-config-libdir=%{MY_PKG} \\\
	--with-shared \\\
	--with-terminfo-dirs=%{MYDATA}:/usr/share/terminfo \\\
	--with-termlib \\\
	--with-ticlib \\\
	--with-trace \\\
	--with-versioned-syms \\\
	--with-xterm-kbs=DEL \\\
	--without-ada \\\
	--without-debug \\\
	--without-normal

CFLAGS="%{CC_NORMAL}" \
RPATH_LIST=../lib:%{_libdir} \
%configure %{CFG_OPTS}

make

%install
rm -rf $RPM_BUILD_ROOT

make install.libs install.progs
rm -f test/ncurses
( cd test && make ncurses LOCAL_LIBDIR=%{_libdir} && mv ncurses $RPM_BUILD_ROOT/%{_bindir}/ncurses%{MY_ABI} )

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/*
%{_includedir}/*
%{_libdir}/*

%changelog

* Sat Aug 25 2018 Thomas E. Dickey
- split spec-file into ncurses6 and ncursest6 to work around toolset breakage
  in Fedora 28

* Sat Jun 02 2018 Thomas E. Dickey
- build-fix for Mageia

* Sat May 26 2018 Thomas E. Dickey
- use predefined configure-macro
- separate ncurses6/ncursest6 packages

* Sat Feb 10 2018 Thomas E. Dickey
- add ncursest6 package
- add several development features

* Mon Jan 01 2018 Thomas E. Dickey
- drop redundant files pattern for "*.pc"

* Tue Dec 26 2017 Thomas E. Dickey
- add --with-config-suffix option

* Sun Apr 26 2015 Thomas E. Dickey
- move package to /usr

* Sun Apr 12 2015 Thomas E. Dickey
- factor-out MY_ABI

* Sat Mar 09 2013 Thomas E. Dickey
- add --with-cxx-shared option to demonstrate c++ binding as shared library

* Sat Oct 27 2012 Thomas E. Dickey
- add ncurses program as "ncurses6" to provide demonstration.

* Fri Jun 08 2012 Thomas E. Dickey
- initial version.
