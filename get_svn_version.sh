#!/bin/sh
ver=`svn info | grep Revision | cut -b11-`
date=`svn info | grep "Last Changed Date" | cut -b20-`
if [ "X$ver" == "X" ]; then
  echo This is not a valid svn checkout
else
  echo \#define SVN_VER \"$ver\" > $1
  echo \#define SVN_DATE \"$date\" >> $1
fi