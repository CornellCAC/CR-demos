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

#define MPI_CHUNK_SIZE 1000
//For DEBUG:
//#define MPI_CHUNK_SIZE 10

//#define REALLOC_SIZE 2
#define REALLOC_SIZE MPI_CHUNK_SIZE

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
MPI_Request perf_diffs_request     = MPI_REQUEST_NULL;

/*
 * State variables
 */ 
big_int     *perf_diffs = NULL;   /* pointer to data buffer to write */
big_int     *perf_diffs_tmp = NULL;
big_int     count, count_in_chunk, num_odd, num_even;
big_int     new_odds, new_evens;

/* Function declarations */

big_int perfect_diff(big_int n);

void backup_file();

void alloc_and_init();

herr_t checkpoint(MPI_Comm comm, MPI_Info info,
                  big_int* perf_diffs, big_int num_even, big_int num_odd, 
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
  // Initialize data offsets for each MPI process
  count = MPI_CHUNK_SIZE * mpi_rank;  
  while(true) {
    count_in_chunk = 0;
    new_evens = 0;
    new_odds  = 0;
    alloc_and_init();
    while(count < MPI_CHUNK_SIZE) {
      perf_diffs[count_in_chunk] = perfect_diff(count);
      if (perf_diffs[count_in_chunk] == 0) {
	printf("Found %lld!\n", count);
	if (count % 2 == 0) {
	  new_evens++;
	} else {
	  new_odds++;
	}
      } // end [if (perfect_diff(count) == 0)]
      count++;
      count_in_chunk++;
    } // end [while(count < MPI_CHUNK_SIZE)]
    // Synchronize metadata state and then save state
    broadcast_state();
    num_even += new_evens;
    num_odd  += new_odds;
    checkpoint(comm, info, perf_diffs, num_even, num_odd, count);    
    // offset into next iteration
    count += (mpi_size - 1) * MPI_CHUNK_SIZE;
  } // end [while(true)]

  free(perf_diffs);

  MPI_Finalize();

  return 0;
}



//
// We don't really need such a complicated function,
// but this form can be useful if you have a dataset
// that may need to grow in memory.
//
void alloc_and_init() {
  int ii;
  perf_diffs_tmp = (big_int *) realloc(perf_diffs, 
    REALLOC_SIZE * sizeof *perf_diffs
  );
  if (!perf_diffs_tmp) {
    /* Could not reallocate, checkpoint and exit */
    fprintf(stderr, "Couldn't allocate more space for perf data!\n");
    fflush(stdout);
    free(perf_diffs);
    MPI_Finalize();
    exit(-1);
  } else {
    /* Initialize data */
    perf_diffs = perf_diffs_tmp;
    for (ii = 0; ii < REALLOC_SIZE; ii++) {
	perf_diffs[ii] = 0;
    }
  }
  return;
}


herr_t checkpoint(MPI_Comm comm, MPI_Info info,                 
                big_int* perf_diffs, big_int num_even, big_int num_odd, 
                big_int last_n) {

  /*
   * HDF5 APIs definitions
   */ 	

  /* property list identifier */
  hid_t	      file_access_plist_id;
  hid_t       dset_plist_id;
  /* object identifiers */
  hid_t       file_id, dset_id, status_id;
  hid_t       last_n_id;
  hid_t       num_even_id;
  hid_t       num_odd_id;
  /* file and memory dataspace identifiers */
  hid_t       filespace;
  hid_t       memspace;
  hid_t       last_n_dataspace_id;
  hid_t       num_even_dataspace_id;
  hid_t       num_odd_dataspace_id;
  /* dataset dimension (just 1 here) */
  hsize_t     dimsf[] = {MPI_CHUNK_SIZE * mpi_size}; 
  hsize_t     dimsm[] = {MPI_CHUNK_SIZE}; 
  /* hyperslab dimension and size */
  hsize_t     count[]  = {MPI_CHUNK_SIZE}; 
  hssize_t    offset[] = {MPI_CHUNK_SIZE * mpi_rank}; 
  /* scalar attribute dimension (just 1 here) */
  hsize_t     attr_dimsf[] = {1}; 
  herr_t      status;

  // Only need to backup once per checkpoint
  if (mpi_rank == 0) {
    if( access( H5FILE_NAME, F_OK ) != -1 ) {
      // File exists already, backup first 
      backup_file();
    }
  }

  // 
  // Set up file access property list with parallel I/O access
  // Also create a new file collectively then release property list identifier
  //
  file_access_plist_id = H5Pcreate(H5P_FILE_ACCESS);	
  H5Pset_fapl_mpio(file_access_plist_id, comm, info);
  file_id = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, file_access_plist_id);
  H5Pclose(file_access_plist_id);

  //
  // Create the dataspace for the dataset.
  //
  filespace = H5Screate_simple(RANK, dimsf, NULL); 

  //
  // Create the memspace for the dataset: think of this
  // as the window each process has into the total dataset.
  //
  memspace = H5Screate_simple(RANK, dimsm, NULL); 

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

  //
  // Create the dataset with default properties.
  //
  dset_id = H5Dcreate(file_id, DATASETNAME, big_int_h5, filespace,
		      H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  assert(dset_id != HDF_FAIL);

  //
  // Select this process's hyperslab
  //
  H5Sselect_hyperslab(filespace, H5S_SELECT_SET, 
                      offset, NULL, count, NULL);

  //
  // Set up (collective) dataset-write property list
  //
  dset_plist_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(dset_plist_id, H5FD_MPIO_COLLECTIVE);


  //
  // Collectively write dataset to disk
  //
  status = H5Dwrite(dset_id, big_int_h5, memspace, filespace,
  		    dset_plist_id, (const void *) perf_diffs);

  //
  // Close/release resources.
  //
  H5Dclose(dset_id);
  H5Pclose(dset_plist_id);
  H5Gclose(status_id);
  H5Sclose(memspace);
  H5Sclose(filespace);
  H5Fclose(file_id);

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
