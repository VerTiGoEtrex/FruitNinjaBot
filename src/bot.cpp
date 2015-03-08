#include "bot.hpp"

using cv::Mat;
using cv::waitKey;

void imageFromDisplay(std::vector<uint8_t>& pixels, int& width, int& height, int& bitsPerPixel) {
    Display* display = XOpenDisplay(nullptr);
    Window root = DefaultRootWindow(display);

    XWindowAttributes attributes = {0};
    XGetWindowAttributes(display, root, &attributes);

    width = attributes.width;
    height = attributes.height;

    XImage* img = XGetImage(display, root, 0, 0 , width, height, AllPlanes, ZPixmap);
    bitsPerPixel = img->bits_per_pixel;
    pixels.resize(width * height * 4);

    std::copy_n(img->data, pixels.size(), pixels.begin());

    XFree(img);
    XCloseDisplay(display);
}

int main()
{
    int width = 0;
    int height = 0;
    int bpp = 0;
    std::vector<std::uint8_t> pixels;

    imageFromDisplay(pixels, width, height, bpp);

    if (width && height)
    {
        Mat img = Mat(height, width, bpp > 24 ? CV_8UC4 : CV_8UC3, &pixels[0]); //Mat(Size(Height, Width), Bpp > 24 ? CV_8UC4 : CV_8UC3, &Pixels[0]);

        //namedWindow("WindowTitle", cv::WINDOW_AUTOSIZE);
        imshow("Display window", img);

        waitKey(0);
    }
    return 0;
}
