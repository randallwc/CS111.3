# NAME: William Randall,Dylan Gunn
# EMAIL: wrandall1000@gmail.com,dylankgunn@gmail.com
# ID: 805167986,805112866

CC=gcc
CFLAGS=-Wall -Wextra
NAME=lab3a
UID=805167986
FILES=README Makefile lab3a.cpp

default: $(NAME).cpp
	$(CC) $(CFLAGS) -o $(NAME) $(NAME).c

clean:
	rm -f $(NAME)
	rm -f $(NAME)-$(UID).tar.gz

dist:
	tar -czf $(NAME)-$(UID).tar.gz $(FILES)