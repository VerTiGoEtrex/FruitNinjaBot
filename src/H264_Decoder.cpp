#include "H264_Decoder.h"


uint64_t rx_hrtime() {
  static clock_t fast_clock_id = -1;
  struct timespec t;
  clock_t clock_id;

  if(fast_clock_id == -1) {
    if(clock_getres(CLOCK_MONOTONIC_COARSE, &t) == 0 && (unsigned) t.tv_nsec <= 1 * 1000 * 1000LLU) {
      fast_clock_id = CLOCK_MONOTONIC_COARSE;
    }
    else {
      fast_clock_id = CLOCK_MONOTONIC;
    }
  }

  clock_id =  CLOCK_MONOTONIC;
  if(clock_gettime(clock_id, &t)) {
    return 0; 
  }

  return t.tv_sec * (uint64_t)1e9 +t.tv_nsec;
}


H264_Decoder::H264_Decoder(h264_decoder_callback frameCallback, void* user)  :codec(NULL)
  ,codec_context(NULL)
  ,parser(NULL)
  ,fp(NULL)
  ,frame(0)
  ,cb_frame(frameCallback)
  ,cb_user(user)
  ,frame_timeout(0)
  ,frame_delay(0)
{
  avcodec_register_all();
}

H264_Decoder::~H264_Decoder() {
  if(parser) {
    av_parser_close(parser);
    parser = NULL;
  }
  if(codec_context) {
    avcodec_close(codec_context);
    //av_free(codec_context); // FIXME
    codec_context = NULL;
  }
  if(picture) {
    avcodec_free_frame(&picture);
    picture = NULL;
  }
  if(fp) {
    fclose(fp);
    fp = NULL;
  }
  cb_frame = NULL;
  cb_user = NULL;
  frame = 0;
  frame_timeout = 0;
}

bool H264_Decoder::load(int fd, float fps) {
  codec = avcodec_find_decoder(AV_CODEC_ID_H264);
  if(!codec) {
    printf("Error: cannot find the h264 codec\n");
    return false;
  }
  codec_context = avcodec_alloc_context3(codec);
  if(codec->capabilities & CODEC_CAP_TRUNCATED) {
    codec_context->flags |= CODEC_FLAG_TRUNCATED;
  }
  if(avcodec_open2(codec_context, codec, NULL) < 0) {
    printf("Error: could not open codec.\n");
    return false;
  }
  fp = fdopen(fd, "rb");
  if(!fp) {
    printf("Error: cannot open\n");
    return false;
  }
  picture = avcodec_alloc_frame();
  parser = av_parser_init(AV_CODEC_ID_H264);
  if(!parser) {
    printf("Erorr: cannot create H264 parser.\n");
    return false;
  }
  if(fps > 0.0001f) {
    frame_delay = (1.0f/fps) * 1000ull * 1000ull * 1000ull;
    frame_timeout = rx_hrtime() + frame_delay;
  }
  // kickoff reading...
  readBuffer();
  return true;
}

bool H264_Decoder::readFrame() {
  uint64_t now = rx_hrtime();
  if(now < frame_timeout) {
    return false;
  }
  bool needs_more = false;
  while(!update(needs_more)) {    if(needs_more) {
      readBuffer();
    }
  }
  // it may take some 'reads' before we can set the fps
  if(frame_timeout == 0 && frame_delay == 0) {
    double fps = av_q2d(codec_context->time_base);
    if(fps > 0.0) {
      frame_delay = fps * 1000ull * 1000ull * 1000ull;
    }
  }
  if(frame_delay > 0) {
    frame_timeout = rx_hrtime() + frame_delay;
  }
  return true;
}

void H264_Decoder::decodeFrame(uint8_t* data, int size) {
  AVPacket pkt;
  int got_picture = 0;
  int len = 0;
  av_init_packet(&pkt);
  pkt.data = data;
  pkt.size = size;
  len = avcodec_decode_video2(codec_context, picture, &got_picture, &pkt);
  if(len < 0) {
    printf("Error while decoding a frame.\n");
  }
  if(got_picture == 0) {
    return;
  }
  ++frame;
  if(cb_frame) {
    cb_frame(picture, &pkt, cb_user);
  }
}

int H264_Decoder::readBuffer() {
  int bytes_read = (int)fread(inbuf, 1, H264_INBUF_SIZE, fp);
  if(bytes_read) {
    std::copy(inbuf, inbuf + bytes_read, std::back_inserter(buffer));
  }
  return bytes_read;
}

bool H264_Decoder::update(bool& needsMoreBytes) {
  needsMoreBytes = false;
  if(!fp) {
    printf("Cannot update .. file not opened...\n");
    return false;
  }
  if(buffer.size() == 0) {
    needsMoreBytes = true;
    return false;
  }
  uint8_t* data = NULL;
  int size = 0;
  int len = av_parser_parse2(parser, codec_context, &data, &size, &buffer[0], buffer.size(), 0, 0, AV_NOPTS_VALUE);
  if(size == 0 && len >= 0) {
    needsMoreBytes = true;
    return false;
  }
  if(len) {
    decodeFrame(&buffer[0], size);
    buffer.erase(buffer.begin(), buffer.begin() + len);
    return true;
  }
  return false;
}
