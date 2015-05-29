#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "mpi.h"

#define HOSTNAME_LEN 100

int main(int argc, char **argv) 
{
  char message[20], hostname[HOSTNAME_LEN];
  int  jj, rank, size, tag = 99;
  MPI_Status status;
  int errno = 0;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0)
    {
      strcpy(message, "Hello, world");
      for (jj = 1; jj < size; jj++) 
	MPI_Send(message, 13, MPI_CHAR, jj, tag, MPI_COMM_WORLD);
    }
  else {
    MPI_Recv(message, 20, MPI_CHAR, 0, tag, MPI_COMM_WORLD, &status);
  }

  errno = gethostname(hostname, HOSTNAME_LEN);
  printf( "Message from process %d on host %s: %.13s\n", 
	  rank, hostname, message);

  //Synchronize output here
  MPI_Barrier(MPI_COMM_WORLD);

  //Proceed to slow counting ...
  unsigned long ii = 0;
  while (1) {
    printf("%d:%lu ", rank, ii);
    ii= ii + 1;
    sleep(2);
    fflush(stdout);
  } 

  MPI_Finalize();
}

