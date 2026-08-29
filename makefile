mandelbrot: main.o 
	gcc main.o -o mandelbrot -lm

main.o: main.c 
	gcc -c main.c

run: mandelbrot
	./mandelbrot

clean:
	rm -f *.o mandelbrot

.PHONY: all run clean