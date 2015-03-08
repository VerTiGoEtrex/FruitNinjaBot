#!/bin/bash
adb shell screenrecord --size 1024x768 --o h264 - | mplayer -
