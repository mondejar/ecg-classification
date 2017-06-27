#!/bin/sh
# file: maninst.sh	G. Moody	16 October 1989
#			Last revised:	29 November 2001
# Bourne shell script for installing WFDB software man pages
#
# Copyright (C) 2001 George B. Moody
#
# This script is normally invoked by make -- see the Makefile in this
# directory.  If you want to put your man pages in a non-standard location,
# edit the case statements below first, or just copy the pages manually.

MAN1=$1
MAN3=$2
MAN5=$3
MAN7=$4
SETPERMISSIONS=$5

case $MAN1 in
   *l) EXT=l ;;
   *n) EXT=n ;;
   *)  EXT=1 ;;
esac

for FILE in *.1
do
   cp $FILE $MAN1/`basename $FILE .1`.$EXT
   $SETPERMISSIONS $MAN1/`basename $FILE .1`.$EXT
done

case $MAN3 in
   *l) EXT=l ;;
   *n) EXT=n ;;
   *)  EXT=3 ;;
esac

for FILE in *.3
do
   cp $FILE $MAN3/`basename $FILE .3`.$EXT
   $SETPERMISSIONS $MAN3/`basename $FILE .3`.$EXT
done

case $MAN5 in
   *l) EXT=l ;;
   *n) EXT=n ;;
   *)  EXT=5 ;;
esac

for FILE in *.5
do
   cp $FILE $MAN5/`basename $FILE .5`.$EXT
   $SETPERMISSIONS $MAN5/`basename $FILE .5`.$EXT
done

case $MAN7 in
   *l) EXT=l ;;
   *n) EXT=n ;;
   *)  EXT=7 ;;
esac

for FILE in *.7
do
   cp $FILE $MAN7/`basename $FILE .7`.$EXT
   $SETPERMISSIONS $MAN7/`basename $FILE .7`.$EXT
done
exit 0
