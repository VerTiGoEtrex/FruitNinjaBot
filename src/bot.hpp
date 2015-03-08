#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <opencv2/opencv.hpp>

void imageFromDisplay(std::vector<uint8_t>& pixels, int& width, int& height, int& bitsPerPixel);
