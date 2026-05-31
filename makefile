
build: mpkg.c
	gcc mpkg.c -o mpkg

debug: mpkg.c
	gcc -g mpkg.c -o mpkg

test: output.pak test.c
	gcc test.c -o test
	./test
