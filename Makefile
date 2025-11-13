CC = clang
CFLAGS = -Wall -Wextra -pedantic -std=c17
EXECUTABLE = chat.out
SOURCE = src/*.c

$(EXECUTABLE): objects
	$(CC) $(CFLAGS) $(SOURCE) -I include -o $(EXECUTABLE)

clean:
	rm -f $(EXECUTABLE)