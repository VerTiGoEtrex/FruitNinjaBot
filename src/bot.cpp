#include "bot.hpp"

using cv::Mat;
using cv::Vec;
using cv::waitKey;
using cv::VideoCapture;
using std::cerr;
using std::cout;
using std::endl;
using std::function;
using std::vector;
using std::string;
using std::thread;
using std::for_each;

typedef Vec<unsigned char, 3> Vec3c;

static const int CONTOURAREA = 175;

void exitError(const char* err) {
  perror(err);
  exit(EXIT_FAILURE);
}

bool inColorRange(const Vec3c& pixel, const vector<unsigned char>& red, const vector<unsigned char>& green, const vector<unsigned char>& blue, const string& msg) {
  bool inRange = pixel[2] >= red[0] && pixel[2] <= red[1] &&
                 pixel[1] >= green[0] && pixel[1] <= green[1] &&
                 pixel[0] >= blue[0] && pixel[0] <= blue[1];
  if(inRange)
    cout << msg << endl;
  return inRange;
}

int num = 0;

void writeSwipeEvent(FILE* monkeyStream, int x1, int y1, int x2, int y2, float sec) {
  cerr << "writing swipe event: " << num++ << endl;
  fprintf(monkeyStream, "d.drag((%d, %d), (%d, %d), %f, 10)\n", x1, y1, x2, y2, sec);
  fflush(monkeyStream);
}

/*
 *void writeSwipeEvent(FILE* adbStream, int x1, int y1, int x2, int y2, int msec) {
 *  cerr << "writing swipe event" << endl;
 *  fprintf(adbStream, "input swipe %d %d %d %d %d &\n", x1, y1, x2, y2, msec);
 *  fflush(adbStream);
 *  cerr << "writing swipe event done" << endl;
 *}
 */

void readFromPipe(int fd) {
  FILE *stream;
  int ch;
  cerr << "reading\n";
  stream = fdopen(fd, "r");
  if (stream == NULL)
    exitError("fdopen() r");
  while ( (ch = getc(stream)) != EOF)
    cout << (char) ch;
  fflush(stdout);
  fclose(stream);
  cerr << "reading done\n";
}

void bootSubprocess(int &pipeFromProc, int &pipeToProc, int childRedirectOut, int childRedirectIn, vector<string> command) {
  int pipeTo[2];
  int pipeFrom[2];
  if (pipe(pipeTo) != 0)
    exitError("pipe() from");
  if (pipe(pipeFrom) != 0)
    exitError("pipe() from");

  pid_t pid = fork();
  static const int forkError = -1;
  static const int forkChild = 0;
  switch (pid) {
    case forkError:
      {
        exitError("fork()");
        break;
      }

    case forkChild:
      {
        // Setup pipe to stdin and from stdout
        if (childRedirectIn == -1) {
          dup2(pipeTo[0], STDIN_FILENO);
        } else {
          dup2(childRedirectIn, STDIN_FILENO);
        }

        if (childRedirectOut == -1) {
          dup2(pipeFrom[1], STDOUT_FILENO);
        } else {
          dup2(childRedirectOut, STDOUT_FILENO);
        }

        // Close unneeded FDs (we already duplicated them into stdin and out
        close(pipeTo[0]);
        close(pipeTo[1]);
        close(pipeFrom[0]);
        close(pipeFrom[1]);

        // Start process
        vector<char*> argv;
        argv.reserve(command.size() + 1); // Last one is null
        for (auto& c : command)
          argv.push_back(strdup(c.c_str()));
        argv.push_back(NULL);
        execvp(argv[0], argv.data());
        exitError("execlp");
        break;
      }

    default: //parent
      {
        // Close unneeded FDs
        close (pipeTo[0]);
        close (pipeFrom[1]);

        pipeFromProc = pipeFrom[0];
        pipeToProc = pipeTo[1];
      }
  }
}


std::string sampleName = "samples/sample";
std::string sampleExtension = ".jpg";
unsigned int uniq = 0;

int iterCount = 0;
cv::BackgroundSubtractorMOG2 bgs = cv::BackgroundSubtractorMOG2(300, 32);

cv::CascadeClassifier bombClassifer = cv::CascadeClassifier("./bombClass.xml");

void callback(AVFrame *frame, AVPacket *pkt, void *user) {
  FILE* adbStream = (FILE*)user;
  //cout << "Got frame!\n";
  AVFrame dst;

  cv::Mat mask;
  cv::Mat m;
  cv::Mat fore;
  memset(&dst, 0, sizeof(dst));

  int w = frame->width, h = frame->height;
  m = cv::Mat(h, w, CV_8UC3);
  dst.data[0] = (uint8_t *)m.data;
  avpicture_fill( (AVPicture *)&dst, dst.data[0], PIX_FMT_BGR24, w, h);

  struct SwsContext *convert_ctx=NULL;
  enum PixelFormat src_pixfmt = (enum PixelFormat)frame->format;
  enum PixelFormat dst_pixfmt = PIX_FMT_BGR24;
  convert_ctx = sws_getContext(w, h, src_pixfmt, w, h, dst_pixfmt,
      SWS_FAST_BILINEAR, NULL, NULL, NULL);
  sws_scale(convert_ctx, frame->data, frame->linesize, 0, h,
      dst.data, dst.linesize);
  sws_freeContext(convert_ctx);

  bgs(m, mask);
  if (mask.rows){
    cv::Mat maskCmp(h, w, CV_8UC1, 128);
    cv::compare(mask, maskCmp, mask, cv::CMP_GT);
    maskCmp /= 256;
    cv::Mat maskCopy;
    cv::merge({mask, mask, mask}, maskCopy);
    m.copyTo(fore, maskCopy);
    imshow("BGMOG", fore);
  }

  // Find objects
  cv::Mat processedFore;
  fore.copyTo(processedFore);
  vector<vector<cv::Point>> contourPoints;
  cv::findContours(mask, contourPoints, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
  for (size_t i = 0; i < contourPoints.size(); ++i) {
    if (cv::contourArea(contourPoints[i]) < CONTOURAREA)
      continue;
    cv::drawContours(processedFore, contourPoints, i, cv::Scalar(255, 0, 255), 1, 8);

    cv::Rect rect = boundingRect(contourPoints[i]);
    cv::rectangle(processedFore, rect.tl(), rect.br(), cv::Scalar(0, 255, 255), 1);

    cv::Mat sample = fore(rect);
    /*
     *if (iterCount % 5 == 0)
     *  //cout << "Writing images!" << endl;
     *  cv::imwrite(sampleName + std::to_string(uniq++) + sampleExtension, sample);
     */

    auto actualArea = cv::contourArea(contourPoints[i]);
    int A = rect.width;
    int B = rect.height;
    double estimatedArea = M_PI * A * B;
    if (estimatedArea < 1e-8)
      continue;
    double error = std::fabs(actualArea - estimatedArea);
    double roundness = std::max(0.0, 1 - error/estimatedArea) * std::min(A, B) / std::max(A, B);
    if (roundness > 0.17){
      //cout << roundness << "\n";
      cv::rectangle(processedFore, rect.tl(), rect.br(), cv::Scalar(0, 255, 255), 1);
    }

    //Try swiping at them, I guess
    if (iterCount % 10 == 0) {
      if (rect.tl().y > 75 && rect.tl().y < 150 && roundness > 0.05) {
        //Check for green pixels
        bool isFruit = true;
        //cout << std::distance(sample.begin<Vec3c>(), sample.end<Vec3c>()) << endl;
        /*
         *for(auto it = sample.begin<Vec3c>(); it != sample.end<Vec3c>(); ++it) {
         *  if(inColorRange(*it, {128, 255}, {0, 40}, {128, 255}, "Found Bomb!") || //Bomb (Purple)
         *      inColorRange(*it, {240, 255}, {240, 255}, {240, 255}, "Found Bomb!")) { //Bomb (White)
         *        isFruit = false;
         *      }
         *}
         */

        if(isFruit) {
          if (rect.width > 2 * rect.height){
            writeSwipeEvent(adbStream,
                rect.tl().x * ((float)1920/640) - 100,
                rect.tl().y * ((float)1081/320),
                rect.br().x * ((float)1920/640) + 100,
                rect.br().y * ((float)1080/320),
                0.05);
          }else {
            writeSwipeEvent(adbStream,
                rect.tl().x * ((float)1920/640),
                rect.tl().y * ((float)1080/320) - 100,
                rect.br().x * ((float)1920/640),
                rect.br().y * ((float)1080/320) + 100,
                0.05);
          }
        } else {
          auto sz = sample.size();
          cout << sample(cv::Rect(sz.width/2, sz.height/2, 1, 1))<< endl;
        }
      }
    }
  }
  imshow("PROCESSED", processedFore);

  // Run classifiers
/*
 *  cv::Mat detectFore;
 *  m.copyTo(detectFore);
 *  vector<cv::Rect> bombs;
 *  bombClassifer.detectMultiScale(m, bombs, 1.1, 2, cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30), cv::Size(100, 100));
 *  cout << "Found " << bombs.size() << " bombs\n";
 *
 *  for (auto &b : bombs) {
 *    cv::rectangle(m, b.tl(), b.br(), cv::Scalar(0, 255, 255), 1);
 *  }
 *  imshow("DETECTED", m);
 */

  waitKey(1);
  ++iterCount;
}

int main(int argc, char *argv[]) {
  int devNull = open("/dev/null", O_WRONLY);
  if (devNull == -1)
    exitError("open() /dev/null");

/*
 *  [> ***** ADB SHELL ***** <]
 *  int adbShellPipes[2];
 *  auto adbShellArgs = vector<string>({"adb", "shell"});
 *  bootSubprocess(adbShellPipes[0], adbShellPipes[1], devNull, -1, adbShellArgs);
 *  close(adbShellPipes[0]);
 *
 *  // Open stream to send events
 *  auto adbStream = fdopen(adbShellPipes[1], "w");
 *  if (adbStream == NULL)
 *    exitError("fdopen() r");
 */

  /* ***** MONKEYRUNNER INTERACTIVE SHELL ***** */
  int monkeyPipes[2];
  auto monkeyArgs = vector<string>({"/home/ncrocker/Android/Sdk/tools/monkeyrunner"});
  bootSubprocess(monkeyPipes[0], monkeyPipes[1], devNull, -1, monkeyArgs);
  close(monkeyPipes[0]);

  // Open stream to send events
  auto monkeyStream = fdopen(monkeyPipes[1], "w");
  if (monkeyStream == NULL)
    exitError("fdopen() r");

  fprintf(monkeyStream, "from com.android.monkeyrunner import MonkeyRunner, MonkeyDevice\n");
  fprintf(monkeyStream, "d = MonkeyRunner.waitForConnection()\n");
  fflush(monkeyStream);
  //sleep(3);

  // FIXME demo event
  //writeSwipeEvent(monkeyStream, 100, 100, 450, 450, 2);
  //sleep(3);

  /* ***** SCREENRECORD AND OPENCV INIT ***** */
  int screenPipes[2];
  auto screenArgs = vector<string>({"adb", "shell", "screenrecord", "--size", "640x360", "--o", "h264", "-"});
  bootSubprocess(screenPipes[0], screenPipes[1], -1, 0, screenArgs);
  close(screenPipes[1]); // We never write anything

  //H264_Decoder dec(callback, adbStream);
  H264_Decoder dec(callback, monkeyStream);
  if (!dec.load(screenPipes[0]))
    exitError("H264 Decoder couldn't load FD");

  while (true)
    dec.readFrame();

  /* ***** CLEANUP ADB SHELL ***** */
  //fclose(adbStream);
  fclose(monkeyStream);

  /* ***** CLEANUP SCREENRECORD ***** */
  // TODO

  return EXIT_SUCCESS;
}
