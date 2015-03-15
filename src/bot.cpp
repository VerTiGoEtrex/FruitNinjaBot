#include "bot.hpp"

using cv::Mat;
using cv::waitKey;
using cv::VideoCapture;
using std::cerr;
using std::cout;
using std::endl;
using std::function;
using std::vector;
using std::string;
using std::thread;

void exitError(const char* err) {
  perror(err);
  exit(EXIT_FAILURE);
}

void writeSwipeEvent(FILE* adbStream, int x1, int y1, int x2, int y2, int msec) {
  cerr << "writing swipe event" << endl;
  fprintf(adbStream, "input swipe %d %d %d %d %d &\n", x1, y1, x2, y2, msec);
  fflush(adbStream);
  cerr << "writing swipe event done" << endl;
}

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

void bootOpenCV(const char* pipePath) {
  cout << "OpenCV creating capture object\n";
  VideoCapture cap;
  cap.set(CV_CAP_PROP_FOURCC, CV_FOURCC('H', '2', '6', '4'));
  sleep(5);
  cout << "OpenCV opening pipe\n";
  cap.open(pipePath);
  cout << "OpenCV opened named pipe\n";
  if (!cap.isOpened())
    exitError("OpenCV couldn't open pipe properly");
  cout << "OpenCV is running!\n";

  /* ***** OPEN CV MAIN LOOP ***** */
  while (true) {
    Mat frame;
    cout << "GETTING FRAME\n";
    cap >> frame;
    cout << "GOT FRAME\n";
    imshow("display", frame);
    if(waitKey(30 >= 0)) break;
  }
}

int main(int argc, char *argv[]) {
  int devNull = open("/dev/null", O_WRONLY);
  if (devNull == -1)
    exitError("open() /dev/null");

  /* ***** ADB SHELL ***** */
  int adbShellPipes[2];
  auto adbShellArgs = vector<string>({"adb", "shell"});
  bootSubprocess(adbShellPipes[0], adbShellPipes[1], devNull, -1, adbShellArgs);
  close(adbShellPipes[0]);

  // Open stream to send events
  auto adbStream = fdopen(adbShellPipes[1], "w");
  if (adbStream == NULL)
    exitError("fdopen() r");

  // FIXME demo event
  writeSwipeEvent(adbStream, 100, 100, 450, 450, 500);

  /* ***** SCREENRECORD AND OPENCV INIT ***** */

  // Setup the named pipe to record to
  const char* cvFifoPath = "/tmp/cvFIFO";
  cout << "Making named pipe" << endl;
  if (mkfifo(cvFifoPath, 0666))
    exitError("mkfifo()");

  // Open the named pipe for reading by OpenCV
  cout << "Created named pipe, running openCV\n";
  thread openCV(bootOpenCV, cvFifoPath);

  // Open the named pipe for writing by screenrecord
  cout << "Opening named pipe" << endl;
  int fifoFD = open(cvFifoPath, O_WRONLY);
  cout << "Opened named pipe" << endl;

  // Record to the named pipe
  int screenPipes[2];
  auto screenArgs = vector<string>({"adb", "shell", "screenrecord", "--size", "640x360", "--o", "h264", "--bugreport", "-"});
  bootSubprocess(screenPipes[0], screenPipes[1], fifoFD, devNull, screenArgs);
  close(screenPipes[0]);
  close(screenPipes[1]); // We never write anything

  openCV.join();


  /* ***** CLEANUP ADB SHELL ***** */
  fclose(adbStream);

  /* ***** CLEANUP SCREENRECORD ***** */
  // TODO
  close(fifoFD);
  unlink(cvFifoPath);

  return EXIT_SUCCESS;
}
