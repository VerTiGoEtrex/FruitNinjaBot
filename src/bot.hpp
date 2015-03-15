#include <cstdint>
#include <vector>
#include <algorithm>
#include <iostream>
#include <functional>
#include <thread>
#include "H264_Decoder.h"
#include <opencv2/opencv.hpp>

extern "C" {
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
}
