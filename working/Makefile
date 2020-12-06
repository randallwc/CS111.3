# NAME: William Randall,Dylan Gunn
# EMAIL: wrandall1000@gmail.com,dylankgunn@gmail.com
# ID: 805167986,805112866

CC=g++
CFLAGS=-Wall -Wextra -g -static-libstdc++
NAME=lab3a
UID=805167986
FILES=README Makefile lab3a.cpp ext2_fs.h

default: $(NAME).cpp ext2_fs.h
	$(CC) $(CFLAGS) -o $(NAME) $(NAME).cpp

clean:
	rm -f $(NAME)
	rm -f $(NAME)-$(UID).tar.gz
	rm -f out.diff out.csv temp.img trivial_sorted.csv

check: default trivial.img trivial.csv
	# create a copy of the image
	cp trivial.img temp.img

	# get sorted output of lab3a on trivial.img
	./lab3a temp.img | sort > out.csv

	# get the sorted output to compare to
	cat trivial.csv | sort > trivial_sorted.csv

	# get diff between both sorted outputs
	-diff -u out.csv trivial_sorted.csv > out.diff

	rm -f temp.img trivial_sorted.csv

dist:
	tar -czf $(NAME)-$(UID).tar.gz $(FILES)
