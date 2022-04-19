OBJ = main.o helper.o list.o
all: demon
demon: $(OBJ)
	gcc $(OBJ) -o demon
$(OBJ): list.h helper.o
.PHONY: clean
clean:
	rm -f *.o demon