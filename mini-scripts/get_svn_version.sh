#!/bin/bash
ver=`svn info 2>/dev/null | grep Revision | cut -b11-`  
date=`svn info 2>/dev/null | grep "Last Changed Date" | cut -b20-` 
if [ "X$ver" == "X" ]; then
  echo \#define SVN_VER \"Not generated from a svn checkout.\" > $1
  echo \#define SVN_DATE \"\" >> $1
else

  #Next two lines read first line of file into 'line'
  if [ -e $1 ]
  then
    exec 3< $1
    read <&3 line
  fi
  newline="#define SVN_VER \"${ver}\""
  if [ "${newline}" != "${line}" ]; then
    # Have to update file
    echo Updating svn header file
    echo \#define SVN_VER \"$ver\" > $1
    echo \#define SVN_DATE \"$date\" >> $1
  fi
fi