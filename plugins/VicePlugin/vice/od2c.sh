#!/bin/sh

# this file takes the input of od -Ax -t x1 -w8 <file> and outputs part 
# of an array

start="yes"

while read data
do
  charlength=${#data}
  datalength=`expr $charlength - 6`
  tl=`expr $datalength / 3`
  if test x"$start" != "xyes"; then
    if [ $tl -ne 0 ]; then
        tdata="$tdata,"
        echo "    $tdata"
    fi
  else
    start=no
  fi
  if [ $tl -ne 0 ]; then
    if [ $tl -ge 1 ]; then
      tdata="0x${data:7:2}"
    fi
    if [ $tl -ge 2 ]; then
      tdata="$tdata, 0x${data:10:2}"
    fi
    if [ $tl -ge 3 ]; then
      tdata="$tdata, 0x${data:13:2}"
    fi
    if [ $tl -ge 4 ]; then
      tdata="$tdata, 0x${data:16:2}"
    fi
    if [ $tl -ge 5 ]; then
      tdata="$tdata, 0x${data:19:2}"
    fi
    if [ $tl -ge 6 ]; then
      tdata="$tdata, 0x${data:22:2}"
    fi
    if [ $tl -ge 7 ]; then
      tdata="$tdata, 0x${data:25:2}"
    fi
    if [ $tl -ge 8 ]; then
      tdata="$tdata, 0x${data:28:2}"
    fi
  fi
done
echo "    $tdata"
