CC=clang++
CFLAGS= -std=c++11 -g -O0 -Wall -Wextra -Werror -Wno-missing-field-initializers -fsanitize=address
IFLAGS= 
LFLAGS=

SRC=src/bsp2obj.cpp src/mesh.cpp src/bspdata.cpp src/indexedimage.cpp src/entityparser.cpp
OBJ=$(SRC:.cpp=.o)

OUTFILE=bsp2obj

app: $(OBJ)
	@$(CC) $(CFLAGS) $(IFLAGS) $(LFLAGS) $^ -o $(OUTFILE)

%.o : %.cpp
	@$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

clean:
	@rm $(OBJ)

.phony: clean
