#!/bin/bash

NAME="classifer"
echo -en "\033]0;$NAME\a"
UNCLASSIFED=./Positives/*
for f in $UNCLASSIFED
do
  eog $f &
  disown
  sleep 1
  wmctrl -a $NAME
  echo "------------------"
  echo "Please classify"
  echo "(R)ed apple"
  echo "(G)reen apple"
  echo "(B)anana"
  echo "(C)oconut"
  echo "(L)emon"
  echo "(O)range"
  echo "(P)ear"
  echo "P(e)ach"
  echo "(S)trawberry"
  echo "(W)atermelon"

  echo "(U)nivNegatives"
  echo "(M)aybeNegatives"
  echo "Input:"
  read -n 1 class
  case $class in
    [r])
      mv $f ./Classified/RedApple/
      ;;
    [g])
      mv $f ./Classified/GreenApple/
      ;;
    [b])
      mv $f ./Classified/Banana/
      ;;
    [c])
      mv $f ./Classified/Coconut/
      ;;
    [l])
      mv $f ./Classified/Lemon/
      ;;
    [o])
      mv $f ./Classified/Orange/
      ;;
    [p])
      mv $f ./Classified/Pear/
      ;;
    [e])
      mv $f ./Classified/Peach/
      ;;
    [s])
      mv $f ./Classified/Strawberry/
      ;;
    [w])
      mv $f ./Classified/Watermelon/
      ;;
    [u])
      mv $f ./Classified/UnivNegatives/
      ;;
    [m])
      mv $f ./Classified/MaybeNegatives/
      ;;
    *)
      echo "INVALID CLASS"
      ;;
  esac
  kill $!
done
