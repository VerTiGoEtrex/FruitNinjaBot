#!/bin/bash
adb shell screenrecord --size 640x360 --o h264 --bugreport - | mplayer -demuxer h264es -
