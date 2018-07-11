#!/bin/bash

##############################################################################
# BASH script check that nlreader stdout redirected to tmp has all messages. #
##############################################################################

FILE='tmp'
CMD=$(cat $FILE | grep pid | awk -F'=|]' 'FNR>=2 && $3!=p+1{print p+1"-"$3-1}{p=$3}')

if [ $CMD ]; then
  echo "Missing sequence number :"
  echo $CMD
else
  echo "No message missing"
fi
