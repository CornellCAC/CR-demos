#include <stdio.h> 
#include "mpi.h"
int main(int argc, char **argv) 
{
  char message[20];
  int  j, rank, size, tag = 99;
  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0)
    {
      strcpy(message, "Hello, world");
      for (j = 1; j < size; j++) 
	MPI_Send(message, 13, MPI_CHAR, j, tag, MPI_COMM_WORLD);
    }
  else {
    MPI_Recv(message, 20, MPI_CHAR, 0, tag, MPI_COMM_WORLD, &status);
  }
  printf( "Message from process %d : %.13s\n", rank, message);

  //Proceed to slow counting ...
  unsigned long i = 0;
  while (1) {
    printf("%lu ", i);
    i = i + 1;
    sleep(1);
    fflush(stdout);
  } 

  MPI_Finalize();
}

