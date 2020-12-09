# NAME: Dylan Gunn,William Randall
# EMAIL: dylankgunn@gmail.com,wrandall1000@gmail.com
# ID: 805112866,805167986

NAME=lab3b
UID=805167986
FILES=README Makefile lab3b.py lab3b.sh

default: $(NAME).py $(NAME).sh
	rm -f $(NAME)
	ln -s $(NAME).sh $(NAME)
	chmod +x $(NAME)

clean:
	rm -f $(NAME)
	rm -f $(NAME)-$(UID).tar.gz

dist: default
	tar -czf $(NAME)-$(UID).tar.gz $(FILES)

test: check
check: default test.sh
	chmod +x test.sh
	./test.sh