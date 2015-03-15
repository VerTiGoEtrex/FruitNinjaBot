#include "bot.hpp"

using cv::Mat;
using cv::waitKey;
using std::cerr;
using std::cout;

void exitError(const char* err) {
  perror(err);
  exit(EXIT_FAILURE);
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

int main(int argc, char *argv[])
{
  // Boot screenrecord on Android and pipe stream into our program
  int pipeTo[2];
  int pipeFrom[2];
  if (pipe(pipeTo) != 0)
    exitError("pipe() from");
  if (pipe(pipeFrom) != 0)
    exitError("pipe() from");

  pid_t screenPid = fork();
  bool forkError = screenPid < 0;
  bool isChild = screenPid == 0;
  if (forkError) {
    exitError("fork() screenrecord");

  } else if (isChild) {
    // Setup pipe to stdin and from stdout
    dup2(pipeTo[0], STDIN_FILENO);
    dup2(pipeFrom[1], STDOUT_FILENO);

    // Close unneeded FDs (we already duplicated them into stdin and out
    close(pipeTo[0]);
    close(pipeTo[1]);
    close(pipeFrom[0]);
    close(pipeFrom[1]);

    // Start screenrecord
    execlp("adb", "adb", "shell", "screenrecord", "--size", "640x360", "--o", "h264", "--bugreport", "-", NULL);
    exitError("execlp");

  } else { // parent
    // Close unneeded FDs (We won't send any more commands to this ADB shell)
    close (pipeTo[0]);
    close (pipeTo[1]);
    close (pipeFrom[1]);

    // Pipe ADB to the shell (to view with mplayer)
    readFromPipe(pipeFrom[0]);
  }

  return 0;
}
