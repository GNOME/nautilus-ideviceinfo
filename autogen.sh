#!/bin/sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
olddir=`pwd`

cd "$srcdir"

aclocal -I m4
libtoolize
autoheader
automake --add-missing
autoconf

cd "$olddir"

if [ -z "$NOCONFIGURE" ]; then
    "$srcdir"/configure "$@"
fi
