/*  
 *  This example writes data to the HDF5 file.
 *  Number of processes is assumed to be 1 or multiples of 2 (up to 8)
 */

#include "mpi.h"
 
#include "hdf5.h"
#include <stdlib.h>
#include <stdbool.h>

#define H5FILE_NAME     "SDS.h5"
#define DATASETNAME 	"IntArray" 
#define RANK   2

#define REALLOC_SIZE 10000000

typedef unsigned long big_int;

bool is_perfect(unsigned long n);

int
main (int argc, char **argv)
{
    /*
     * HDF5 APIs definitions
     */ 	
    hid_t       file_id, dset_id;              /* file and dataset identifiers */
    hid_t       filespace;                     /* file and memory dataspace identifiers */
    hsize_t     dimsf[] = {REALLOC_SIZE, 1};   /* dataset dimensions */
    big_int     *data, *data_tmp;              /* pointer to data buffer to write */
    hid_t	plist_id;                      /* property list identifier */
    big_int     ii, count;
    herr_t      status;

    /*
     * MPI variables
     */
    int mpi_size, mpi_rank;
    MPI_Comm comm  = MPI_COMM_WORLD;
    MPI_Info info  = MPI_INFO_NULL;

    /*
     * Initialize MPI
     */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &mpi_size);
    MPI_Comm_rank(comm, &mpi_rank);  
 
    count = 0;
    while(true) {
      count++;
      /*
       * Initialize data buffer 
       */
      data_tmp = (big_int *) realloc(data, (count + REALLOC_SIZE) * sizeof *data);
      if (!data_tmp) {
	/* Could not reallocate, checkpoint and exit */
      } else {
	data = data_tmp;
	for (ii = count; ii < count + REALLOC_SIZE; ii++) {
	    data[ii] = 0;
	}
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
      dset_id = H5Dcreate(file_id, DATASETNAME, H5T_NATIVE_INT, filespace,
			  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      /*
       * Create property list for collective dataset write.
       */
      plist_id = H5Pcreate(H5P_DATASET_XFER);
		 H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

      /*
       * To write dataset independently use
       *
       * H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_INDEPENDENT); 
       */

      status = H5Dwrite(dset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
			plist_id, data);

    }

    free(data);

    /*
     * Close/release resources.
     */
    H5Dclose(dset_id);
    H5Sclose(filespace);
    H5Pclose(plist_id);
    H5Fclose(file_id);
 
    MPI_Finalize();

    return 0;
}


// Tells if a given integer is perfect
// 
// This is toy code. Odd perfect numbers are currently being searched
// for above 10^300: http://www.oddperfect.org
// Also see http://rosettacode.org/wiki/Perfect_numbers
// 
bool is_perfect(big_int n) {
  big_int divisor_product = 1;
  big_int i = 1;
  for (i = 2; i < n; i++) {
    if (n % i == 0) {
      divisor_product *= i;
    }
  }
  if (divisor_product == n) {
    return true;
  }
  return false;
}
