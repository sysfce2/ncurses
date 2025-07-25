#!/bin/sh
# $Id: make-tar.sh,v 1.26 2025/06/20 19:42:53 tom Exp $
##############################################################################
# Copyright 2019-2022,2025 Thomas E. Dickey                                  #
# Copyright 2010-2015,2017 Free Software Foundation, Inc.                    #
#                                                                            #
# Permission is hereby granted, free of charge, to any person obtaining a    #
# copy of this software and associated documentation files (the "Software"), #
# to deal in the Software without restriction, including without limitation  #
# the rights to use, copy, modify, merge, publish, distribute, distribute    #
# with modifications, sublicense, and/or sell copies of the Software, and to #
# permit persons to whom the Software is furnished to do so, subject to the  #
# following conditions:                                                      #
#                                                                            #
# The above copyright notice and this permission notice shall be included in #
# all copies or substantial portions of the Software.                        #
#                                                                            #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    #
# THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        #
# DEALINGS IN THE SOFTWARE.                                                  #
#                                                                            #
# Except as contained in this notice, the name(s) of the above copyright     #
# holders shall not be used in advertising or otherwise to promote the sale, #
# use or other dealings in this Software without prior written               #
# authorization.                                                             #
##############################################################################
# Construct a tar-file containing only the Ada95 tree as well as its associated
# documentation.  The reason for doing that is to simplify distributing the
# ada binding as a separate package.

CDPATH=:
export CDPATH

TARGET=`pwd`

: "${ROOTNAME:=ncurses-Ada95}"
: "${PKG_NAME:=AdaCurses}"
: "${DESTDIR:=$TARGET}"
: "${TMPDIR:=/tmp}"

# make timestamps of generated files predictable
same_timestamp() {
	[ -f ../NEWS ] || echo "OOPS $1"
	touch -r ../NEWS "$1"
}

grep_assign() {
	grep_assign=`grep -E "^$2\>" "$1" | sed -e "s/^$2[ 	]*=[ 	]*//" -e 's/"//g'`
	eval "$2"=\""$grep_assign"\"
}

grep_patchdate() {
	grep_assign ../dist.mk NCURSES_MAJOR
	grep_assign ../dist.mk NCURSES_MINOR
	grep_assign ../dist.mk NCURSES_PATCH
}

# The rpm spec-file in the ncurses tree is a template.  Fill in the version
# information from dist.mk
edit_specfile() {
	RPM_DATE=`date +'%a %b %d %Y' -d "$NCURSES_PATCH"`
	sed \
		-e "s/\\<MAJOR\\>/$NCURSES_MAJOR/g" \
		-e "s/\\<MINOR\\>/$NCURSES_MINOR/g" \
		-e "s/\\<YYYYMMDD\\>/$NCURSES_PATCH/g" \
		-e "s/\\<RPM_DATE\\>/$RPM_DATE/g" "$1" >"$1.new"
	chmod u+w "$1"
	mv "$1.new" "$1"
	same_timestamp "$1"
}

make_changelog() {
	[ -f "$1" ] && chmod u+w "$1"
	cat >"$1" <<EOF
`echo "$PKG_NAME"|tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' 'abcdefghijklmnopqrstuvwxyz'` ($NCURSES_MAJOR.$NCURSES_MINOR+$NCURSES_PATCH) unstable; urgency=low

  * snapshot of ncurses subpackage for $PKG_NAME.

 -- `head -n 1 "$HOME"/.signature`  `date -R`
EOF
	same_timestamp "$1"
}

# This can be run from either the subdirectory, or from the top-level
# source directory.  We will put the tar file in the original directory.
if [ -d ./Ada95 ]
then
	cd ./Ada95 || exit
fi
SOURCE=`cd ..;pwd`

BUILD=$TMPDIR/make-tar$$
trap "cd /; rm -rf $BUILD; exit 1" 1 2 3 15
trap "cd /; rm -rf $BUILD; exit 0" 0

umask 077
if ! ( mkdir "$BUILD" )
then
	echo "? cannot make build directory $BUILD"
fi

umask 022
mkdir "$BUILD/$ROOTNAME"

cp -p -r ./* "$BUILD/$ROOTNAME/" || exit

# Add the config.* utility scripts from the top-level directory.
for i in . ..
do
	for j in COPYING config.guess config.sub install-sh tar-copy.sh
	do
		[ -f "$i/$j" ] && cp -p "$i/$j" "$BUILD/$ROOTNAME"/
	done
done

# Make rpm and dpkg scripts for test-builds
grep_patchdate
for spec in "$BUILD/$ROOTNAME"/package/*.spec
do
	edit_specfile "$spec"
done
for spec in "$BUILD/$ROOTNAME"/package/debian*
do
	make_changelog "$spec"/changelog
done

cp -p ../man/MKada_config.in "$BUILD/$ROOTNAME/doc/"
if [ -z "$NO_HTML_DOCS" ]
then
	# Add the ada documentation.
	cd ../doc/html || exit

	cp -p -r Ada* "$BUILD/$ROOTNAME/doc/"
	cp -p -r ada "$BUILD/$ROOTNAME/doc/"

	cd ../../Ada95 || exit
fi

cp -p "$SOURCE/NEWS" "$BUILD/$ROOTNAME"

# cleanup empty directories (an artifact of ncurses source archives)

touch "$BUILD/$ROOTNAME"/MANIFEST
( cd "$BUILD/$ROOTNAME" && find . -type f -print | "$SOURCE/misc/csort" >MANIFEST )
same_timestamp "$BUILD/$ROOTNAME"/MANIFEST

cd "$BUILD" || exit

# Remove build-artifacts.
find . -name RCS -exec rm -rf {} \;
find "$BUILD/$ROOTNAME" -type d -exec rmdir {} \; 2>/dev/null
find "$BUILD/$ROOTNAME" -type d -exec rmdir {} \; 2>/dev/null
find "$BUILD/$ROOTNAME" -type d -exec rmdir {} \; 2>/dev/null

# There is no need for this script in the tar file.
rm -f "$ROOTNAME"/make-tar.sh

# Remove build-artifacts.
find . -name "*.gz" -exec rm -rf {} \;

# Make the files writable...
chmod -R u+w .

# Cleanup timestamps
[ -n "$TOUCH_DIRS" ] && "$TOUCH_DIRS" "$ROOTNAME"

tar cf - $TAR_OPTIONS "$ROOTNAME" | gzip >"$DESTDIR/$ROOTNAME.tar.gz"
cd "$DESTDIR" || exit

pwd
ls -l "$ROOTNAME".tar.gz

# vi:ts=4 sw=4
