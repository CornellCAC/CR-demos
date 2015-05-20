/*  
 *  This example writes data to the HDF5 file.
 *  Number of processes is assumed to be 1 or multiples of 2 (up to 8)
 */

#include "mpi.h"
 
#include "hdf5.h"

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define H5FILE_NAME     "perfectNumbers.h5"
#define H5FILE_BACKUP   "perfectNumbers.h5.bak"
#define BACKUP_CMD      "/bin/cp " H5FILE_NAME " " H5FILE_BACKUP
#define DATASETNAME 	"BigIntArray"  
#define STATUSGROUP 	"status"
#define RANK            1   

#define REALLOC_SIZE 100
#define MPI_CHUNK_SIZE 10000000

#define big_int_h5 H5T_NATIVE_ULLONG
typedef unsigned long big_int;


  /*
   * MPI variables
   */
  int mpi_size, mpi_rank;
  MPI_Comm comm  = MPI_COMM_WORLD;
  MPI_Info info  = MPI_INFO_NULL;


/* Function declarations */

bool is_perfect(unsigned long n);

void backup_file();

herr_t checkpoint(MPI_Comm comm, MPI_Info info,
                  big_int* data, big_int num_even, big_int num_odd, 
                  big_int last_perf, big_int last_n);


int main (int argc, char **argv)
{
  big_int     *data = NULL;   /* pointer to data buffer to write */
  big_int     *data_tmp = NULL;
  big_int     count, last_perf, num_odd, num_even;
  int         ii; 

  /*
   * Initialize MPI
   */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(comm, &mpi_size);
  MPI_Comm_rank(comm, &mpi_rank);  

  last_perf = 0;
  num_even = 0;
  num_odd = 0;
  // Initialize data offsets for each MPI process
  for(ii = 0; ii < mpi_size, ii++;) {
    if (mpi_rank == ii) {
      count = MPI_CHUNK_SIZE * mpi_rank;  
    }
  }
  while(true) {
    while(count < count + MPI_CHUNK_SIZE) {
      printf("Count is %llu!\n", count);
      if (is_perfect(count)) {
	last_perf = count;
	printf("Found %llu!\n", last_perf);

	// Might need to allocate more memory to hold new perfect number
	if ((num_even + num_odd) % REALLOC_SIZE == 0) {
	  /*
	   * Initialize data buffer 
	   */

	  // TODO: note that variable length data arrays can't be used with *p*HDF5
	  data_tmp = (big_int *) realloc(data, 
	    (num_even + num_odd + REALLOC_SIZE) * sizeof *data
	  );
	  if (!data_tmp) {
	    /* Could not reallocate, checkpoint and exit */
	    fprintf(stderr, "Couldn't allocate more space for data!\n");
	    fflush(stdout);
	    free(data);
	    MPI_Finalize();
	    exit(-1);

	  } else {
            /* Initialize data */
	    data = data_tmp;
	    for (ii = num_even + num_odd; ii < num_even + num_odd + REALLOC_SIZE; ii++) {
		data[ii] = 0;
	    }
	  }
	} // end of [if ((num_even + num_odd) % REALLOC_SIZE == 0)]

	if (last_perf % 2 == 0) {
	  num_even++;
	} else {
	  num_odd++;
	}
	data[num_even + num_odd - 1] = last_perf;
	// TODO: (for exercise) should ALSO checkpoint based on count, to account for 
	// long gaps in perfect numbers
	checkpoint(comm, info, data, num_even, num_odd, 
		   last_perf, count);
      } // end [if (is_perfect(count))]
      count++;
    } // end [while(count < count + MPI_CHUNK_SIZE)]
    // offset into next iteration
    count += (mpi_size - 1) * MPI_CHUNK_SIZE + 1; 
  } // end [while(true)]

  free(data);

  MPI_Finalize();

  return 0;
}


herr_t checkpoint(MPI_Comm comm, MPI_Info info,                 
                big_int* data, big_int num_even, big_int num_odd, 
                big_int last_perf, big_int last_n) {

  /*
   * HDF5 APIs definitions
   */ 	

  /* property list identifier */
  hid_t	      plist_id;
  /* object identifiers */
  hid_t       file_id, dset_id, status_id;
  hid_t       last_perf_id;
  hid_t       last_n_id;
  hid_t       num_even_id;
  hid_t       num_odd_id;
  /* file and memory dataspace identifiers */
  hid_t       filespace;
  hid_t       last_perf_dataspace_id;
  hid_t       last_n_dataspace_id;
  hid_t       num_even_dataspace_id;
  hid_t       num_odd_dataspace_id;
  /* dataset dimension (just 1 here) */
  hsize_t     dimsf[] = {num_even + num_odd}; 
  /* scalar attribute dimension (just 1 here) */
  hsize_t     attr_dimsf[] = {1}; 
  herr_t      status;


  if( access( H5FILE_NAME, F_OK ) != -1 ) {
    /* File exists already, backup first */
    backup_file();
  }

  /* 
   * Set up file access property list with parallel I/O access
   */
  plist_id = H5Pcreate(H5P_FILE_ACCESS);	
	     H5Pset_fapl_mpio(plist_id, comm, info);

  /*
   * Create a new file collectively and release property list identifier.
   */
  file_id = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
	    H5Pclose(plist_id);


  /*
   * Create the dataspace for the dataset.
   */
  filespace = H5Screate_simple(RANK, dimsf, NULL); 

  /*
   * Create the dataset with default properties and close filespace.
   */
  dset_id = H5Dcreate(file_id, DATASETNAME, big_int_h5, filespace,
		      H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /*
   * Create status group
   */
  status_id = H5Gcreate(file_id, STATUSGROUP,
	                H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  // Only need to write once, and send the last_n of lowest offset
  if (rank == (mpi_size - 1)) {
    /*
     * Add metadata as attributes 
     */
    last_perf_dataspace_id = H5Screate_simple(1, attr_dimsf, NULL);
    last_perf_id = H5Acreate(status_id, "Last Perfect Number", big_int_h5, 
			     last_perf_dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(last_perf_id, big_int_h5, &last_perf);
    status = H5Aclose(last_perf_id);
    status = H5Sclose(last_perf_dataspace_id);
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
  }

  /*
   * Create property list for collective dataset write.
   */
  plist_id = H5Pcreate(H5P_DATASET_XFER);
	     H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

  status = H5Dwrite(dset_id, big_int_h5, H5S_ALL, H5S_ALL,
		    plist_id, (const void *) data);

  /*
   * Close/release resources.
   */
  H5Dclose(dset_id);
  H5Gclose(status_id);
  H5Sclose(filespace);
  H5Pclose(plist_id);
  H5Fclose(file_id);

  return status;

}



// Tells if a given integer is a perfect number: the sum of its
// divisors (excluding itself) is equal to itself.
// 
// This is toy code. Odd perfect numbers are currently being searched
// for above 10^300: http://www.oddperfect.org
// Also see : http://rosettacode.org/wiki/Perfect_numbers
//          : http://en.wikipedia.org/wiki/List_of_perfect_numbers
bool is_perfect(big_int n) {
  if (n < 2) {
    return false;
  }
  big_int divisor_sum = 1;
  big_int i;
  for (i = 2; i < n; i++) {
    if (n % i == 0) {
      divisor_sum += i;
    }
  }
  if (divisor_sum == n) {
    return true;
  }
  return false;
}



void backup_file() {
  system(BACKUP_CMD);
}
