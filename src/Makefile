CXXFLAGS := -Wall -Werror -g -std=c++11
LDFLAGS := `pkg-config --cflags --libs libswscale opencv libavcodec`

BIN = bot
THREADLIB =

all: $(BIN)

bot: bot.o H264_Decoder.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *~ *.out *.o $(BIN)
