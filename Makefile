.PHONY: clean test

toyforth.exe: toyforth.c
	gcc toyforth.c -Wall -W -O2 -o toyforth.exe
clean: toyforth.exe
	rm toyforth.exe
test: toyforth.exe program.tf
	./toyforth.exe program.tf
