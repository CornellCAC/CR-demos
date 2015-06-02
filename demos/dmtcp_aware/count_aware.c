#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

/* Be sure to compile with -I<path>; see Makefile in this directory. */
#include "dmtcp.h"

#define INTS_PER_LOOP 3

// Prints a sequence of n integers starting at 0 
// at a rate of 1 character integer second, then checkpoints

int main(int argc, char* argv[])
{
  unsigned long ii = 0;
  int count = 0;
  int rr;
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
    do {
      printf("%d ", ii);
      fflush(stdout);
      sleep(1);
      ii++;
    } while (ii % INTS_PER_LOOP != 0);
    printf("\n");
    // Checkpoint and print result
    if(dmtcp_is_enabled()){
      printf("\n");
      rr = dmtcp_checkpoint();
      if(rr == DMTCP_NOT_PRESENT)
	printf("***** Error, DMTCP not running; checkpoint skipped ***** \n");
      if(rr == DMTCP_AFTER_CHECKPOINT)
	printf("***** after checkpoint *****\n");
      if(rr == DMTCP_AFTER_RESTART)
	printf("***** after restart *****\n");
    }else{
      printf(" dmtcp disabled -- nevermind\n");
    }

  }
  return 0;
}
