/*  
 *  This example writes data to the HDF5 file.
 *  Number of processes is assumed to be 1 or multiples of 2 (up to 8)
 */

#include "mpi.h"
 
#include "hdf5.h"

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define H5FILE_NAME     "perfectNumbers.h5"
#define H5FILE_BACKUP   "perfectNumbers.h5.bak"
#define BACKUP_CMD      "/bin/cp " H5FILE_NAME " " H5FILE_BACKUP
#define DATASETNAME 	"BigIntArray"  
#define STATUSGROUP 	"status"
#define RANK            1   

#define REALLOC_SIZE 2
#define MPI_CHUNK_SIZE 1000
//For DEBUG:
//#define MPI_CHUNK_SIZE 10

#define HDF_FAIL -1

#define big_int_h5 H5T_NATIVE_LLONG
#define big_int_mpi MPI_LONG_LONG
typedef long long big_int;

/*
 * MPI variables
 */
int mpi_size, mpi_rank;
MPI_Comm comm  = MPI_COMM_WORLD;
MPI_Info info  = MPI_INFO_NULL;
MPI_Status status;
MPI_Request num_even_request  = MPI_REQUEST_NULL;
MPI_Request num_odd_request   = MPI_REQUEST_NULL;
MPI_Request perfs_request     = MPI_REQUEST_NULL;

/*
 * State variables
 */ 
big_int     *perfs = NULL;   /* pointer to data buffer to write */
big_int     *perfs_tmp = NULL;
big_int     count, count_start, num_odd, num_even;
big_int     new_odds, new_evens;

/* Function declarations */

big_int perfect_diff(big_int n);

void backup_file();

void check_and_realloc(big_int num_even, big_int num_odd);

herr_t checkpoint(MPI_Comm comm, MPI_Info info,
                  big_int* perfs, big_int num_even, big_int num_odd, 
                  big_int last_n);

int broadcast_state();

int wait_for_state();

int main (int argc, char **argv)
{
  int         ii; 

  /*
   * Initialize MPI
   */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(comm, &mpi_size);
  MPI_Comm_rank(comm, &mpi_rank);  

  num_even = 0;
  num_odd = 0;
  check_and_realloc(num_even, num_odd);
  // Initialize data offsets for each MPI process
  count = MPI_CHUNK_SIZE * mpi_rank;  
  while(true) {
    printf("Count is %lld on rank %d!\n", count, mpi_rank);
    count_start = count;
    new_evens = 0;
    new_odds = 0;
    while(count < count_start + MPI_CHUNK_SIZE) {
      if (count_start == 0) {
        printf("Minor count is %lld!\n", count);
      }
      if (perfect_diff(count) == 0) {
        //wait_for_state();
	printf("Found %lld!\n", count);
	// May need to allocate more memory to hold new perfect number
        check_and_realloc(num_even, num_odd);
	if (count % 2 == 0) {
	  new_evens++;
	} else {
	  new_odds++;
	}
      } // end [if (perfect_diff(count) == 0)]
      count++;

    } // end [while(count < count + MPI_CHUNK_SIZE)]
    // Initiate synchronization
    broadcast_state();
    num_even += new_evens;
    num_odd += new_odds;
    checkpoint(comm, info, perfs, num_even, num_odd, count);
    
    // offset into next iteration
    count += (mpi_size - 1) * MPI_CHUNK_SIZE; 
  } // end [while(true)]

  free(perfs);

  MPI_Finalize();

  return 0;
}


void check_and_realloc(big_int num_even, big_int num_odd) {
  int ii;
  if ((num_even + num_odd) % REALLOC_SIZE == 0) {
    /*
     * Initialize data buffer 
     */

    // TODO: note that variable length data arrays can't be used with *p*HDF5
    perfs_tmp = (big_int *) realloc(perfs, 
      (num_even + num_odd + REALLOC_SIZE) * sizeof *perfs
    );
    if (!perfs_tmp) {
      /* Could not reallocate, checkpoint and exit */
      fprintf(stderr, "Couldn't allocate more space for perf data!\n");
      fflush(stdout);
      free(perfs);
      MPI_Finalize();
      exit(-1);
    } else {
      /* Initialize data */
      perfs = perfs_tmp;
      for (ii = num_even + num_odd; ii < num_even + num_odd + REALLOC_SIZE; ii++) {
	  perfs[ii] = 0;
      }
    }
  } // end of [if ((num_even + num_odd) % REALLOC_SIZE == 0)]
  return;
}


herr_t checkpoint(MPI_Comm comm, MPI_Info info,                 
                big_int* perfs, big_int num_even, big_int num_odd, 
                big_int last_n) {

  /*
   * HDF5 APIs definitions
   */ 	

  /* property list identifier */
  hid_t	      file_access_plist_id;
  /* object identifiers */
  hid_t       file_id, dset_id, status_id;
  hid_t       last_n_id;
  hid_t       num_even_id;
  hid_t       num_odd_id;
  /* file and memory dataspace identifiers */
  hid_t       filespace;
  hid_t       last_n_dataspace_id;
  hid_t       num_even_dataspace_id;
  hid_t       num_odd_dataspace_id;
  /* dataset dimension (just 1 here) */
  hsize_t     dimsf[] = {num_even + num_odd}; 
  /* scalar attribute dimension (just 1 here) */
  hsize_t     attr_dimsf[] = {1}; 
  herr_t      status;

  if( access( H5FILE_NAME, F_OK ) != -1 ) {
    // File exists already, backup first 
    backup_file();
  }

  // 
  // Set up file access property list with parallel I/O access
  //
  file_access_plist_id = H5Pcreate(H5P_FILE_ACCESS);	
  H5Pset_fapl_mpio(file_access_plist_id, comm, info);
  file_id = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, file_access_plist_id);
  H5Pclose(file_access_plist_id);
//  if (mpi_rank == 0) {

    //
    // Create a new file collectively and release property list identifier.
    //


    //
    // Create the dataspace for the dataset.
    //
    filespace = H5Screate_simple(RANK, dimsf, NULL); 
    printf("Dims of rank %d: %d\n", mpi_rank, dimsf[0]);

    //
    // Create the dataset with default properties and close filespace.
    //

    printf("Before dset_id on rank %d!\n", mpi_rank);
    dset_id = H5Dcreate(file_id, DATASETNAME, big_int_h5, filespace,
			H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    printf("After dset_id on rank %d!\n", mpi_rank);
    assert(dset_id != HDF_FAIL);

    //
    // Create status group
    //
    status_id = H5Gcreate(file_id, STATUSGROUP,
			  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    //
    // Add metadata as attributes 
    //
    last_n_dataspace_id = H5Screate_simple(1, attr_dimsf, NULL);
    last_n_id = H5Acreate(status_id, "Last Number Tested ", big_int_h5, 
			  last_n_dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(last_n_id, big_int_h5, &last_n);
    status = H5Aclose(last_n_id);
    status = H5Sclose(last_n_dataspace_id);
    //
    num_even_dataspace_id = H5Screate_simple(1, attr_dimsf, NULL);
    num_even_id = H5Acreate(status_id, "Number of even perfect numbers found", 
			    big_int_h5, num_even_dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(num_even_id, big_int_h5, &num_even);
    status = H5Aclose(num_even_id);
    status = H5Sclose(num_even_dataspace_id);
    //
    num_odd_dataspace_id = H5Screate_simple(1, attr_dimsf, NULL);
    num_odd_id = H5Acreate(status_id, "Number of odd perfect numbers found", big_int_h5, 
			   num_odd_dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(num_odd_id, big_int_h5, &num_odd);
    status = H5Aclose(num_odd_id);
    status = H5Sclose(num_odd_dataspace_id);



    //plist_id = H5Pcreate(H5P_DATASET_XFER);
    //H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

    //status = H5Dwrite(dset_id, big_int_h5, H5S_ALL, H5S_ALL,
    //		    plist_id, (const void *) perfs);  //plist_id or H5P_DEFAULT 

    //
    // Close/release resources.
    //
    H5Dclose(dset_id);
    printf("After dset_id CLOSE on rank %d!\n", mpi_rank);
    H5Gclose(status_id);
    H5Sclose(filespace);
    printf("After filespace CLOSE on rank %d!\n", mpi_rank);
//} 
  H5Fclose(file_id);
  printf("After file_id CLOSE on rank %d!\n", mpi_rank);

  printf("After plist_id CLOSE on rank %d!\n", mpi_rank);

  //TODO: move these to non parallel block above
  /*
   * Create property list for collective dataset write.
   */

  //plist_id = H5Pcreate(H5P_DATASET_XFER);
  //H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_INDEPENDENT /*H5FD_MPIO_COLLECTIVE*/);

  //status = H5Dwrite(dset_id, big_int_h5, H5S_ALL, H5S_ALL,
  //    plist_id, (const void *) perfs);


  return status;

}



// Tells if a given integer is a perfect number by returning the
// difference between itself and the sum of its
// divisors (excluding itself); if the diff is 0, it is perfect.
// 
// This is toy code. Odd perfect numbers are currently being searched
// for above 10^300: http://www.oddperfect.org
// Also see : http://rosettacode.org/wiki/Perfect_numbers
//          : http://en.wikipedia.org/wiki/List_of_perfect_numbers
big_int perfect_diff(big_int n) {
  big_int divisor_sum = 0;
  big_int i;
  for (i = 1; i < n; i++) {
    if (n % i == 0) {
      divisor_sum += i;
    }
  }
  return divisor_sum - n;
}



/*
 * Backs up the last checkpoint; very primitive for now.
 */
void backup_file() {
  system(BACKUP_CMD);
}


/*
 * Broadcasts state updates
 */
int broadcast_state() {
  MPI_Allreduce(MPI_IN_PLACE, (void *) &new_evens,  1, big_int_mpi, MPI_SUM, comm);
  MPI_Allreduce(MPI_IN_PLACE, (void *) &new_odds,   1, big_int_mpi, MPI_SUM, comm);
  return 0; 
}
