OBJ = main.o helper.o list.o
all: demon
demon: $(OBJ)
	gcc $(OBJ) -o demon
$(OBJ): list.h helper.h
.PHONY: clean
clean:
	rm -f *.o demon generator
generate:
	gcc list.h list.c helper.h helper.c generator.c -o generator
	./generator