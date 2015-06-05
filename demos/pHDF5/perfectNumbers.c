/*  
 *  This example writes data to the HDF5 file.
 *  Number of processes is assumed to be 1 or multiples of 2 (up to 8)
 */

#include "mpi.h"
 
#include "hdf5.h"

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define H5FILE_NAME     "perfectNumbers.h5"
#define H5FILE_BACKUP   "perfectNumbers.h5.bak"
#define BACKUP_CMD      "/bin/cp " H5FILE_NAME " " H5FILE_BACKUP
#define DATASETNAME 	"DifferenceFromPerfect"  
#define STATUSGROUP 	"status"
#define RANK            1   

#define MPI_CHUNK_SIZE 100

#define HDF_FAIL -1

#define big_int_h5 H5T_NATIVE_LLONG
#define big_int_mpi MPI_LONG_LONG
typedef long long big_int;

//
// MPI variables
//
int mpi_size, mpi_rank;
MPI_Comm comm  = MPI_COMM_WORLD;
MPI_Info info  = MPI_INFO_NULL;
MPI_Status status;
MPI_Request num_even_request  = MPI_REQUEST_NULL;
MPI_Request num_odd_request   = MPI_REQUEST_NULL;
MPI_Request perf_diffs_request     = MPI_REQUEST_NULL;

//
// State variables
// 
big_int     *perf_diffs = NULL;   /* pointer to data buffer to write */
big_int     counter, chunk_counter, current_size;
big_int     num_odd, num_even;
big_int     new_odds, new_evens;

//
// Function declarations 

big_int perfect_diff(big_int n);

void perf_update_loopbody(int index);

void backup_file();

big_int* alloc_and_init(big_int *data, big_int realloc_size);

herr_t checkpoint(MPI_Comm comm, MPI_Info info, big_int* perf_diffs);

herr_t restore(MPI_Comm comm, MPI_Info info);

int broadcast_state();

big_int get_restore_chunk_counter(hsize_t datasize);

int wait_for_state();

int main (int argc, char **argv)
{
  int         index;
  big_int     realloc_size;
  /*
   * Initialize MPI
   */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(comm, &mpi_size);
  MPI_Comm_rank(comm, &mpi_rank);  

  num_even = 0;
  num_odd = 0;
  chunk_counter = 0;

  // Restore if prior h5 file exists:
  if( access( H5FILE_NAME, F_OK ) != -1 ) {
    restore(comm, info);
  } else {
    // Initialize data offsets for each MPI process
    counter = MPI_CHUNK_SIZE * mpi_rank;
    chunk_counter++;
    current_size = chunk_counter * MPI_CHUNK_SIZE;
    perf_diffs = alloc_and_init(perf_diffs, current_size);  
  }
  while(true) {
    new_evens = new_odds = 0;
    do {
      perf_update_loopbody(index);
      counter++;
    } while ((counter % MPI_CHUNK_SIZE) != 0);
    // Synchronize metadata state and then save state
    broadcast_state();
    num_even += new_evens;
    num_odd  += new_odds;
    checkpoint(comm, info, perf_diffs);
    // offset into next iteration
    counter += (mpi_size - 1) * MPI_CHUNK_SIZE;
    //
    chunk_counter++;
    current_size = chunk_counter * MPI_CHUNK_SIZE;
    perf_diffs = alloc_and_init(perf_diffs, current_size);     
  } // end [while(true)]

  free(perf_diffs);

  MPI_Finalize();

  return 0;
}


//
// Interior of do-while loop and also used in restore function;
// checks for perfect numbers and updates process-local state
//
void perf_update_loopbody(int index) {  
  index = MPI_CHUNK_SIZE * (chunk_counter-1) + counter % MPI_CHUNK_SIZE;
  perf_diffs[index] = perfect_diff(counter);
  if (perf_diffs[index] == 0) {
    printf("Found %lld!\n", counter);
    if (counter % 2 == 0) {
      new_evens++;
    } else {
      new_odds++;
    }
  } // end [if (perfect_diff(count) == 0)]
  //DEBUG: for illustrative puproses, just write out ints in order
  //perf_diffs[index] = counter;
}

//
// Grow the memory for our dataset as needed, initializing
// new entries
// 
big_int* alloc_and_init(big_int *data, big_int realloc_size) {
  int ii;
  big_int *data_tmp = NULL;
  data_tmp = (big_int *) realloc(data, realloc_size * sizeof *data);
  if (!data_tmp) {
    // Could not reallocate, checkpoint and exit
    fprintf(stderr, "Couldn't allocate more space for data!\n");
    fflush(stdout);
    checkpoint(comm, info, perf_diffs);
    free(data);
    MPI_Finalize();
    exit(-1);
  } else {
    // Initialize data 
    data = data_tmp;
    for (ii = realloc_size - MPI_CHUNK_SIZE; ii < realloc_size; ii++) {
	data[ii] = 0;
    }
  }
  return data;
}


herr_t checkpoint(MPI_Comm comm, MPI_Info info, big_int* perf_diffs) {

  //
  // HDF5 APIs definitions
  //  	

  // property list identifier
  hid_t	      file_access_plist_id;
  hid_t       dset_plist_mpio_id;
  hid_t       dset_plist_create_id;
  // object identifiers 
  hid_t       file_id, dset_id, status_id;
  hid_t       num_even_id;
  hid_t       num_odd_id;
  // file and memory dataspace identifiers 
  hid_t       filespace;
  hid_t       memspace;
  hid_t       num_even_dataspace_id;
  hid_t       num_odd_dataspace_id;
  // dataset and memoryset dimensions (just 1d here) 
  hsize_t     dimsm[]     = {chunk_counter * MPI_CHUNK_SIZE}; 
  hsize_t     dimsf[]     = {dimsm[0] * mpi_size};
  hsize_t     maxdims[]   = {H5S_UNLIMITED};
  hsize_t     chunkdims[] = {1};
  // hyperslab offset and size info */
  hsize_t     start[]   = {mpi_rank * MPI_CHUNK_SIZE};
  hsize_t     count[]   = {chunk_counter};
  hsize_t     block[]   = {MPI_CHUNK_SIZE};
  hsize_t     stride[]  = {MPI_CHUNK_SIZE * mpi_size};
 
  // scalar attribute dimension (just 1 here) 
  hsize_t     attr_dimsf[] = {1}; 
  herr_t      status;

  big_int fillvalue = 0;

  //
  // Need to block terminating signals during I/O
  // Not threadsafe; for threads, use alterantive like pthread_sigmask
  //
  sigset_t sig_list;
  sigemptyset (&sig_list);
  sigaddset(&sig_list, SIGINT);
  sigaddset(&sig_list, SIGTERM);
  sigprocmask(SIG_BLOCK, &sig_list, NULL);

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

  //
  // Create the dataspace for the dataset.
  //
  filespace = H5Screate_simple(RANK, dimsf, maxdims); 

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
  num_even_dataspace_id = H5Screate_simple(1, attr_dimsf, NULL);
  num_even_id = H5Acreate(status_id, "Number of even perfect numbers found", 
			  big_int_h5, num_even_dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
  status = H5Awrite(num_even_id, big_int_h5, &num_even);
  assert(status != HDF_FAIL);
  status = H5Aclose(num_even_id);
  status = H5Sclose(num_even_dataspace_id);
  //
  num_odd_dataspace_id = H5Screate_simple(1, attr_dimsf, NULL);
  num_odd_id = H5Acreate(status_id, "Number of odd perfect numbers found", big_int_h5, 
			 num_odd_dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
  status = H5Awrite(num_odd_id, big_int_h5, &num_odd);
  assert(status != HDF_FAIL);
  status = H5Aclose(num_odd_id);
  status = H5Sclose(num_odd_dataspace_id);

  //
  // Create the dataset with chunk properties (primiarly to 
  // allow variable-sized datasets upon restore).
  //
  dset_plist_create_id = H5Pcreate (H5P_DATASET_CREATE);
  status = H5Pset_chunk (dset_plist_create_id, RANK, chunkdims);
  status = H5Pset_fill_value(dset_plist_create_id, big_int_h5, &fillvalue);
  dset_id = H5Dcreate (file_id, DATASETNAME, big_int_h5, filespace,
		       H5P_DEFAULT, dset_plist_create_id, H5P_DEFAULT);
  assert(dset_id != HDF_FAIL);

  //
  // Select this process's hyperslab
  //
  H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start, stride, count, block);

  //
  // Set up (collective) dataset-write property list
  //
  dset_plist_mpio_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(dset_plist_mpio_id, H5FD_MPIO_COLLECTIVE);


  //
  // Collectively write dataset to disk
  //
  status = H5Dwrite(dset_id, big_int_h5, memspace, filespace,
  		    dset_plist_mpio_id, (const void *) perf_diffs);
  assert(status != HDF_FAIL);

  //
  // Close/release resources.
  //
  H5Dclose(dset_id);
  H5Pclose(dset_plist_create_id);
  H5Pclose(dset_plist_mpio_id);
  H5Gclose(status_id);
  H5Sclose(memspace);
  H5Sclose(filespace);
  H5Fclose(file_id);

  // I/O finished; resume handling of cancellation signals
  sigprocmask(SIG_UNBLOCK, &sig_list, NULL);

  return status;
}

herr_t restore(MPI_Comm comm, MPI_Info info) {

  //
  // HDF5 APIs definitions
  // 	

  // property list identifier 
  hid_t	      file_access_plist_id;
  hid_t       dset_plist_mpio_id;
  // object identifiers 
  hid_t       file_id, dset_id, status_id;
  hid_t       num_even_id;
  hid_t       num_odd_id;
  // file and memory dataspace identifiers 
  hid_t       filespace;
  hid_t       memspace;
  // dataset and memoryset dimensions (just 1d here) 
  hsize_t     dimsm[1]; 
  hsize_t     dimsf[1]; 
  // hyperslab offset and size info 
  hsize_t     start[]   = {mpi_rank * MPI_CHUNK_SIZE};
  hsize_t     count[1];
  hsize_t     block[]   = {MPI_CHUNK_SIZE};
  hsize_t     stride[]  = {MPI_CHUNK_SIZE * mpi_size}; 
  // scalar attribute dimension (just 1 here)
  hsize_t     attr_dimsf[] = {1}; 
  herr_t      status;

  // 
  // Set up file access property list with parallel I/O access
  // Also create a new file collectively then release property list identifier
  // Note we use H5F_ACC_RDWR so we can change the dataset (extent) size
  // 
  file_access_plist_id = H5Pcreate(H5P_FILE_ACCESS);	
  H5Pset_fapl_mpio(file_access_plist_id, comm, info);
  file_id = H5Fopen(H5FILE_NAME, H5F_ACC_RDWR, file_access_plist_id);
  H5Pclose(file_access_plist_id);

  //
  // Open the existing dataset with default properties
  //
  dset_id = H5Dopen(file_id, DATASETNAME, H5P_DEFAULT);
  assert(dset_id != HDF_FAIL);

  //
  // Create the dataspace for the dataset and infer its size
  //
  filespace = H5Dget_space(dset_id);
  H5Sget_simple_extent_dims(filespace, dimsf, NULL);

  //
  // Update dimensions and dataspaces as appropriate
  //
  chunk_counter = get_restore_chunk_counter(dimsf[0]);
  count[0] = chunk_counter;
  dimsm[0] = chunk_counter * MPI_CHUNK_SIZE;
  counter = ((chunk_counter - 1) * mpi_size + mpi_rank) * MPI_CHUNK_SIZE;
  current_size = dimsm[0];
  dimsf[0] = dimsm[0] * mpi_size;
  status = H5Dset_extent(dset_id, dimsf);
  assert(status != HDF_FAIL);

  //
  // Create the memspace for the dataset and allocate data for it 
  //
  memspace = H5Screate_simple(RANK, dimsm, NULL); 
  perf_diffs = alloc_and_init(perf_diffs, dimsm[0]);

  //
  // Open status group
  //
  status_id = H5Gopen(file_id, STATUSGROUP, H5P_DEFAULT);

  //
  // Read metadata attributes 
  //
  num_even_id = H5Aopen(status_id, "Number of even perfect numbers found", H5P_DEFAULT);
  status = H5Aread(num_even_id, big_int_h5, &num_even);
  assert(status != HDF_FAIL);
  status = H5Aclose(num_even_id);
  //
  num_odd_id = H5Aopen(status_id, "Number of odd perfect numbers found", H5P_DEFAULT); 
  status = H5Aread(num_odd_id, big_int_h5, &num_odd);
  assert(status != HDF_FAIL);
  status = H5Aclose(num_odd_id);

  //
  // Select this process's hyperslab
  //
  H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start, stride, count, block);

  //
  // Set up (collective) dataset-read property list
  //
  dset_plist_mpio_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(dset_plist_mpio_id, H5FD_MPIO_COLLECTIVE);

  //
  // Collectively read dataset from disk
  //
  status = H5Dread(dset_id, big_int_h5, memspace, filespace,
  		   dset_plist_mpio_id, (void *) perf_diffs);
  assert(status != HDF_FAIL);

  //FIXME: update count for each process

  //
  // Close/release resources.
  //
  H5Dclose(dset_id);
  H5Pclose(dset_plist_mpio_id);
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


//
// Backs up the last checkpoint; very primitive for now.
//
void backup_file() {
  system(BACKUP_CMD);
}


//
// Broadcasts state updates
//
int broadcast_state() {
  MPI_Allreduce(MPI_IN_PLACE, (void *) &new_evens,  1, big_int_mpi, MPI_SUM, comm);
  MPI_Allreduce(MPI_IN_PLACE, (void *) &new_odds,   1, big_int_mpi, MPI_SUM, comm);
  return 0; 
}

//
// Calculates the number of chunks to allocate
// upon a restore
//
big_int get_restore_chunk_counter(hsize_t datasize) {
  big_int total_chunks = datasize / MPI_CHUNK_SIZE;
  if ((datasize % MPI_CHUNK_SIZE) != 0) {
    total_chunks++;
  }
  if ((total_chunks % mpi_size) != 0) {
    return total_chunks / mpi_size + 1;
  } else {
    return total_chunks / mpi_size;
  }
}

