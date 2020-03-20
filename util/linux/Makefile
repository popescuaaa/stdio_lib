build: utility.h constant_values.h so_stdio.h
	gcc -Wall -g -fPIC -c so_stdio.c utility.h constant_values.h so_stdio.h
	gcc -shared so_stdio.o -o libso_stdio.so

clean:
	rm -rf so_stdio.so libso_stdio.so
