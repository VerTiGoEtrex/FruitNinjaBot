extern "C" {
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
}
//using namespace std;

int num = 0;

const char* emptyTimeval = "\0\0\0\0\0\0\0\0";
long timeVal = 0;
unsigned int touchId = 50;

void sendTimestamp(FILE* adbStream) {
  fwrite(&timeVal, sizeof(int), 2, adbStream);
  timeVal++;
  fflush(adbStream);
}

void sendTrackingId(FILE* adbStream, int id) {
  sendTimestamp(adbStream);
  fwrite("\x03\x00\x39\x00", sizeof(char), 4, adbStream);
  // (randomized) id
  fwrite(&id, sizeof(char), 4, adbStream);
  fflush(adbStream);
}

void sendXPosition(FILE* adbStream, int x) {
  sendTimestamp(adbStream);
  fwrite("\x03\x00\x35\x00", sizeof(char), 4, adbStream);
  fwrite(&x, sizeof(char), 4, adbStream);
  fflush(adbStream);
}

void sendYPosition(FILE* adbStream, int y) {
  sendTimestamp(adbStream);
  fwrite("\x03\x00\x36\x00", sizeof(char), 4, adbStream);
  fwrite(&y, sizeof(char), 4, adbStream);
  fflush(adbStream);
}

void sendPressure(FILE* adbStream, int pressure) {
  sendTimestamp(adbStream);
  fwrite("\x03\x00\x3a\x00", sizeof(char), 4, adbStream);
  fwrite(&pressure, sizeof(char), 4, adbStream);
  fflush(adbStream);
}

void sendSynReport(FILE* adbStream) {
  sendTimestamp(adbStream);
  fwrite(emptyTimeval, sizeof(char), 8, adbStream);
  fflush(adbStream);
}

void startSwipeEvent(FILE* adbStream, int x, int y) {
  //cerr << "starting swipe event" << endl;
  sendTrackingId(adbStream, touchId);
  sendXPosition(adbStream, x);
  sendYPosition(adbStream, y);
  sendPressure(adbStream, 0x36);
  sendSynReport(adbStream);
}

void stopSwipeEvent(FILE* adbStream) {
  //cerr << "stopping swipe event" << endl;
  sendTrackingId(adbStream, -1);
  sendSynReport(adbStream);
}

void swipeWaypoint(FILE* adbStream, int x, int y) {
  sendXPosition(adbStream, x);
  sendYPosition(adbStream, y);
  sendSynReport(adbStream);
}

int lerp(int a, int b, float d) {
  return (b-a) * d + a;
}

long int getMs() {
  timeval tv;
  gettimeofday(&tv, NULL);
  long int start = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  return start;
}

void linterp(FILE* adbStream, int x1, int y1, int x2, int y2, long dur) {
  long now = getMs();
  long start = now;
  long end = start + dur;
  while (now < end) {
    long elapsed = now - start;
    float d = (float) elapsed/dur;
    swipeWaypoint(stdout, lerp(x1, x2, d), lerp(y1, y2, d));
    now = getMs();
  }
}

int main () {
  startSwipeEvent(stdout, 50, 50);
  linterp(stdout, 50, 50, 500, 500, 1000);
  stopSwipeEvent(stdout);
  return 0;
}
