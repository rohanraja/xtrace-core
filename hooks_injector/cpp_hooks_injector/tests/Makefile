CC = g++
CFLAGS = -Wall -Wextra -std=c++11 -DLOCAL_DEV \
 -I /Users/rohanraja/src/codevines/ide1/parser/cpp/CppRecorder 
#  -I /Users/rohanraja/src/codevines/ide1/third_party/rapidjson/include
SRC = $(wildcard out.cc)
OBJ = $(SRC:.cc=.o)

all: main

main: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f main $(OBJ)

run:
	make clean && make && ./main