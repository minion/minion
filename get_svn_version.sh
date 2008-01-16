#!/bin/bash
ver=`svn info | grep Revision | cut -b11-`
date=`svn info | grep "Last Changed Date" | cut -b20-`
if [ "X$ver" == "X" ]; then
  echo This is not a valid svn checkout
else

  #Next two lines read first line of file into 'line'
  exec 3< $1
  read <&3 line
  newline="#define SVN_VER \"${ver}\""
  if [ "${newline}" != "${line}" ]; then
    # Have to update file
    echo Updating svn header file
    echo \#define SVN_VER \"$ver\" > $1
    echo \#define SVN_DATE \"$date\" >> $1
  fi
fi