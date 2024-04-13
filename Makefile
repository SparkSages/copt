CC = gcc
CFLAGS = -O0
CFLAGS2 = -O2
COPT_EXE = copt
COPT_EXE2 = copt2

copt: copt_fun.o copt.o
	$(CC) $(CFLAGS) copt_fun.o copt.o -o copt

copt2: copt_fun2.o copt.o
	$(CC) $(CFLAGS2) copt_fun2.o copt.o -o copt2

copt.o: copt.c copt.h
	$(CC) $(CFLAGS) -c copt.c -o copt.o

copt_fun.o: copt_fun.c copt_fun.h
	$(CC) $(CFLAGS) -c copt_fun.c -o copt_fun.o

copt_fun2.o: copt_fun.c copt_fun.h
	$(CC) $(CFLAGS2) -c copt_fun.c -o copt_fun2.o

test: copt
	./$(COPT_EXE) 0 3000 200; echo ""
	./$(COPT_EXE) 1 300000 20000; echo ""
	./$(COPT_EXE) 2 20 200000000; echo ""
	./$(COPT_EXE) 3 1600 1; echo ""

test2: copt2
	./$(COPT_EXE2) 0 3000 200; echo ""
	./$(COPT_EXE2) 1 300000 20000; echo ""
	./$(COPT_EXE2) 2 20 200000000; echo ""
	./$(COPT_EXE2) 3 1600 1; echo ""

test_mat_init: copt copt2
	./$(COPT_EXE) 0 3000 50; echo ""
	./$(COPT_EXE2) 0 3000 50; echo ""
	./$(COPT_EXE) 0 3000 200; echo ""
	./$(COPT_EXE2) 0 3000 200; echo ""

test_arr_init: copt
	./$(COPT_EXE) 1 30000 2000; echo ""

test_fact: copt
	./$(COPT_EXE) 2 20 200000000; echo ""

test_mat_mult: copt
	./$(COPT_EXE) 3 800 1; echo ""

clean :
	-rm *.o $(COPT_EXE) $(COPT_EXE2)
