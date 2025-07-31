#!/bin/sh
muddir="/home/luminari/mud"
binname="circle"
cd $muddir
process=`ps auxwww | grep circle | grep " 4100" | grep -v grep | awk '{print $11}'`
if [ -z "$process" ]; then
  ulimit -c unlimited
  killall sleep
  killall autorun-luminari-pp
  TODAYSDATE=$(date +"%m-%d-%Y")
  FNAME="syslog.$TODAYSDATE"
  TIMEDATE=$(date +"%m-%d-%Y-%H-%M")
  DUMPNAME="core.$TIMEDATE"
  mv lib/core dumps/$DUMPNAME
  cp syslog $FNAME
  ./autorun-luminari-pp &
  exit
else
  echo Server is already up
  exit
fi

