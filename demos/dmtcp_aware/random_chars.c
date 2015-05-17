#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

/* Be sure to compile with -I<path>; see Makefile in this directory. */
#include "dmtcp.h"

#define MAXCHARS 8

// Prints a random alphabetical string between 1 and MAXCHARS 
// at a rate of 1 character per second, then checkpoints

int main(int argc, char* argv[])
{
  int count = 0;
  int n;
  int r;
  int numCheckpoints, numRestarts;
  while (1)
  {
    if(dmtcp_is_enabled()){
      dmtcp_get_local_status(&numCheckpoints, &numRestarts);
      printf("on iteration %d: this process has checkpointed %d times and restarted %d times\n",
             ++count, numCheckpoints, numRestarts);
    }else{
      printf("on iteration %d; DMTCP not enabled!\n", ++count);
    }
    n = rand() % MAXCHARS + 1;
    int i;
    for (i = 0; i < n; i++) {
      printf("%d", i);
      fflush(stdout);
      sleep(1);
    }
    // Checkpoint and print result
    if(dmtcp_is_enabled()){
      printf("\n");
      r = dmtcp_checkpoint();
      if(r<=0)
	printf("Error, checkpointing failed: %d\n",r);
      if(r==1)
	printf("***** after checkpoint *****\n");
      if(r==2)
	printf("***** after restart *****\n");
    }else{
      printf(" dmtcp disabled -- nevermind\n");
    }

  }
  return 0;
}
