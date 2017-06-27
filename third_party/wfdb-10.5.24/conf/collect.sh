#!/bin/sh

# file: collect.sh	G. Moody	11 May 2006
#
# This script collects the specified files from the specified source
# directory and places copies of them in a like-named directory under
# /tmp/wfdb, creating that directory if it does not exist already.
# The copies have the same creation dates and permissions as the originals.

if [ $# -lt 2 ]
then
 echo usage: $0 SRCDIR FILE ...; exit 1
fi

SRC=$1
DST=/tmp/wfdb/$1

[ -d $DST ] || mkdir -p $DST

shift
for FILE in $*
do
  cp -p $SRC/$FILE $DST/$FILE
done

exit 0

