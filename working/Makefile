# NAME: William Randall,Dylan Gunn
# EMAIL: wrandall1000@gmail.com,dylankgunn@gmail.com
# ID: 805167986,805112866

CC=g++
CFLAGS=-Wall -Wextra -g
NAME=lab3a
UID=805167986
FILES=README Makefile lab3a.cpp ext2_fs.h

default: $(NAME).cpp ext2_fs.h
	$(CC) $(CFLAGS) -o $(NAME) $(NAME).cpp

clean:
	rm -f $(NAME)
	rm -f $(NAME)-$(UID).tar.gz

check: default trivial.img trivial.csv
	# create a copy of the image
	cp trivial.img temp.img

	# get sorted output of lab3a on trivial.img
	./lab3a temp.img | sort > out.csv

	# get the sorted output to compare to
	cat trivial.csv | sort > sorted.csv

	# get diff between both sorted outputs
	diff -u out.csv sorted.csv > out.diff

	# delete image copy
	rm -rf temp.img sorted.csv

dist:
	tar -czf $(NAME)-$(UID).tar.gz $(FILES)
