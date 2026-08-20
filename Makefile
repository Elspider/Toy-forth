.PHONY: clean test advanced_test

toyforth.exe: toyforth.c
	gcc toyforth.c -Wall -W -O2 -o toyforth.exe
clean: toyforth.exe
	rm toyforth.exe
test: toyforth.exe program.tf
	./toyforth.exe program.tf
advanced_test: toyforth.exe program.tf
	@echo "Normal test"
	./toyforth.exe program.tf
	@echo "advanced test"
	valgrind ./toyforth.exe program.tf

