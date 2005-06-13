#!/bin/sh
# script to prepare bzflag sources
autoreconf --install --force || exit 1

if [ -z "$1" ] ; then
 echo BZFlag sources are now prepared. To build here, run:
 echo " ./configure"
 echo " make"
else
 ./configure $*
 make
fi
