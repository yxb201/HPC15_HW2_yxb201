CC=mpicc

all: mpi_solved1 mpi_solved2 mpi_solved3 mpi_solved4 mpi_solved5 mpi_solved6 mpi_solved7 \
     ssort \
     hw2writeup 

mpi_solved1: mpi_solved1.c
	$(CC) mpi_solved1.c -o mpi_solved1

mpi_solved2: mpi_solved2.c
	$(CC) mpi_solved2.c -o mpi_solved2

mpi_solved3: mpi_solved3.c
	$(CC) mpi_solved3.c -o mpi_solved3

mpi_solved4: mpi_solved4.c
	$(CC) mpi_solved4.c -o mpi_solved4

mpi_solved5: mpi_solved5.c
	$(CC) mpi_solved5.c -o mpi_solved5

mpi_solved6: mpi_solved6.c
	$(CC) mpi_solved6.c -o mpi_solved6

mpi_solved7: mpi_solved7.c
	$(CC) mpi_solved7.c -o mpi_solved7

ssort: ssort.c
	$(CC) ssort.c -o ssort

hw2writeup: hw2writeup.tex
	latex hw2writeup.tex
	dvipdfm hw2writeup.dvi


clean:
	rm  mpi_solved1 mpi_solved2 mpi_solved3 mpi_solved4 mpi_solved5 mpi_solved6 mpi_solved7 ssort ssort*.txt *.log *.aux *.pdf *.dvi *.out
