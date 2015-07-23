PROGRAM = wavinfo
LOADLIBES = -lglut -lm -lpthread -ljpeg
CXX.SRCS = wavinfo.c bmpview.c events.c wavedec.c wavelets.c v4l2control.c
CC = gcc
DEFINES = 
CXX = gcc
CPPFLAGS = -O3 -Wall $(DEFINES)
OBJS = $(CXX.SRCS:.c=.o)
all : $(PROGRAM)
$(PROGRAM) : $(OBJS)
	$(CC) $(CPPFLAGS) $(OBJS) $(LOADLIBES) -o $(PROGRAM)
clean:
	/bin/rm -f *.o *~
depend:
	$(CXX) -MM $(CXX.SRCS)

### <DEPENDENCIES ON .h FILES GO HERE> 
# name1.o : header1.h header2.h ...
