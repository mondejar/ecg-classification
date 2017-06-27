#!/bin/sh
# file: uninstall.sh		G. Moody	5 June 2000

case $# in
 0) echo "usage: $0 DIRECTORY [FILE ...]"
      exit ;;
esac

DIR=$1
if [ -d $DIR ]
then
    cd $DIR
    shift
    for FILE in $*
    do
        rm -f $FILE
    done
    cd ..

    # Note that the rmdir command below will fail if DIR is not empty.
    # This is a *feature*, not a bug -- don't fix it!
    rmdir $DIR || echo "(Ignored)"
else
    echo "uninstall: $DIR does not exist"
fi
