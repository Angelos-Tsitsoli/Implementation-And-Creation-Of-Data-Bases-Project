objects= test.o record.o
record : $(objects)
	gcc $(objects) -o test
test.o: record.h
clean:
	rm test $(objects);