#!/bin/sh
#
# MKterminfo.sh -- generate terminfo.5 from Caps tabular data
#
# This script takes terminfo.head and terminfo.tail and splices in between
# them a table derived from the Caps master file.  Besides avoiding having
# the docs fall out of sync with the table, this also lets us set up tbl
# commands for better formatting of the table.
#
# NOTE: The s in this script really are control characters.  It translates
#  to \n because I couldn't get used to inserting linefeeds directly.  There
# had better be no s in the table source text.
#
head=$1
caps=$2
tail=$3
cat <<'EOF'
'\" t
.\" DO NOT EDIT THIS FILE BY HAND!
.\" It is generated from terminfo.head, Caps, and terminfo.tail.
.\"
.\" Note: this must be run through tbl before nroff.
.\" The magic cookie on the first line triggers this under some man programs.
EOF
cat $head
sed -n <$caps "\
/%%-STOP-HERE-%%/q
/^#%/s///p
/^#/d
s/$/T}/
s/	[Y\-][B\-][C\-][G\-][E\-]\**	/	T{/
s/	bool	/	/p
s/	num	/	/p
s/	str	/	/p
" | tr "" "\012"
cat $tail