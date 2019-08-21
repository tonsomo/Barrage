# Project: Barrage
# Makefile created by Dev-C++ 4.9.9.2


CROSS_COMPILE = C:/devkitGP2X/bin/arm-linux-
SDL_BASE = C:/devkitGP2X/bin/arm-linux-
LDFLAGS = -static

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
STRIP = $(CROSS_COMPILE)strip

CFLAGS = `$(SDL_BASE)sdl-config --cflags` -O2 -Wall -DHAVE_CONFIG_H -DAUDIO_ENABLED
CXXFLAGS = `$(SDL_BASE)sdl-config --cflags` -O2 -Wall -DHAVE_CONFIG_H -DAUDIO_ENABLED
LIBS = `$(SDL_BASE)sdl-config --libs` -lSDL_mixer -lsmpeg -lvorbisfile -lvorbis -logg -lmikmod -lSDL -lpthread 

SDLTEST_TARGET = barrage.gpe
SDLTEST_OBJS = gp2x_joystick.o bfield.o chart.o data.o main.o menu.o particle.o SFont.o shots.o units.o

all : $(SDLTEST_TARGET)

$(SDLTEST_TARGET) : $(SDLTEST_OBJS)
	$(CXX) $(LDFLAGS) -o $(SDLTEST_TARGET) $(SDLTEST_OBJS) $(LIBS)
	$(STRIP) $(SDLTEST_TARGET)

clean:
	rm -f $(ALL_TARGETS) *.o *~
