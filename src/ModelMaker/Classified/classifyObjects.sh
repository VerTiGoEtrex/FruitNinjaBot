#!/bin/bash

NAME="classifer"
echo -en "\033]0;$NAME\a"
UNCLASSIFED=./samples/*
for f in $UNCLASSIFED
do
  eog $f &
  disown
  sleep 0.5s
  wmctrl -a $NAME
  echo "------------------"
  echo "Please classify"
  echo "(B)omb"
  echo "(N)ot bomb"
  echo "Input:"
  read -n 1 class
  case $class in
    [b])
      mv $f ./Classified/Bomb/
      ;;
    [n])
      mv $f ./Classified/NotBomb/
      ;;
    *)
      echo "INVALID CLASS"
      ;;
  esac
  kill $!
done
