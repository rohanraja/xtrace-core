CC = g++
CFLAGS = -Wall -Wextra -std=c++11 -I ../../../third_party/rapidjson/include
SRC = $(wildcard *.cc)
OBJ = $(SRC:.cc=.o)

all: main.out

main.out: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f main.out $(OBJ)

run:
	make clean && make && ./main.out