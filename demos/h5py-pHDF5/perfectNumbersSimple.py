#!/usr/bin/python

import os
import sys

import h5py
import numpy as np

# ###    declarations and global variables    ###
# but don't worry about these much yet. Even though we aren't using MPI yet,
# we can go ahead and define how many integers will be evaluated by a single
# process (just one process for now) during each iteration of the outer
# while loop. We define this amount in MPI_CHUNK_SIZE.
H5FILE_NAME    = "perfectNumbers.h5"
DATASETNAME    = "DifferenceFromPerfect"
STATUSGROUP    = "status"
NUM_EVEN_ATTR  = "Number of even perfect numbers found"
NUM_ODD_ATTR   = "Number of odd perfect numbers found"

RANK           = 1   
MPI_CHUNK_SIZE = 100

#
# State variables
#
num_even = 0
num_odd  = 0
perf_diffs = np.zeros(0, dtype=int)
#
chunk_counter = 0
counter       = 0
current_size  = 0

def perfect_diff(n):
    divisor_sum = 1
    for i in range(2, n/2 + 1): 
        if n % i == 0:
            divisor_sum += i
    return divisor_sum - n

def checkpoint():
    # Define our dataset dimensions in dimsf open an HDF5 file for writing,
    # then create our dataset dset_id.
    dimsf  = (chunk_counter * MPI_CHUNK_SIZE,)
    file_id = h5py.File(H5FILE_NAME, "w")
    dset_id = file_id.create_dataset(DATASETNAME, shape=dimsf, dtype='i8')

    # Save some attributes to a group name
    status_id = file_id.create_group(STATUSGROUP)
    status_id.attrs[NUM_EVEN_ATTR] = num_even
    status_id.attrs[NUM_ODD_ATTR]  = num_odd
    # In reality, you'd probably be better off attaching the attributes directly
    # to the dataset (dset_id rather than the status_id group) since the attributes
    # describe the dataset, not the group, but this works well enough for
    # our simple example.

    # Finally, we store everything in perf_diffs to the dataset and close the file.
    dset_id[...] = perf_diffs
    file_id.close()

def restore():
    # We have to declare many global variables. In Python, you need
    # to declare any global variable as global if you intend to write
    # to it, and there are many state variables that need to be restored.
    # In a larger program, you would probably want to avoid a larger mess
    # by having a state object: the fewer global variables, the better.
    global chunk_counter
    global counter
    global current_size
    global num_even
    global num_odd
    global perf_diffs

    # perform some function calls that are analogous to what we saw in
    # the checkpoint function to initiate file and dataset objects and
    # the dimension of the dataset.
    file_id = h5py.File(H5FILE_NAME, "r")    
    dset_id = file_id[DATASETNAME]

    #  We can now restore state, and close the file.
    dimsf = dset_id.shape
    chunk_counter = dimsf[0] / MPI_CHUNK_SIZE
    counter = (chunk_counter - 1) * MPI_CHUNK_SIZE
    current_size = dimsf[0]
    perf_diffs = dset_id[...]
 
    num_even = file_id[STATUSGROUP].attrs[NUM_EVEN_ATTR]
    num_odd  = file_id[STATUSGROUP].attrs[NUM_ODD_ATTR]

    file_id.close()

def main(argv=None):
    if argv is None:
        argv = sys.argv

    global chunk_counter
    global counter
    global current_size
    global num_even
    global num_odd
    global perf_diffs

    #Restore if prior file exists:
    if os.path.isfile(H5FILE_NAME):
        restore()
    else:
        chunk_counter += 1
        current_size  = chunk_counter * MPI_CHUNK_SIZE
        perf_diffs.resize(current_size)

    # We also need to loop over positive integers. This could be simple,
    # but due to the need to split up the work between multiple MPI processes
    # and save data, our while loop grows in complexity:

    #Loop over chunks of size MPI_CHUNK_SIZE and checkpoint each time
    while True:
        # Loop over individual integers (defined by counter)
        while True:
            index = MPI_CHUNK_SIZE * (chunk_counter-1) + counter % MPI_CHUNK_SIZE
            perf_diffs[index] = perfect_diff(counter) 
            if (perf_diffs[index] == 0):
                print("Found %d!\n" % counter)
                if counter % 2 == 0:
                    num_even += 1
                else:
                    num_odd += 1
            # perf_diffs[index] = counter # For DEBUG
            counter += 1
            if counter % MPI_CHUNK_SIZE == 0:
                break
        checkpoint()
        chunk_counter += 1
        current_size = chunk_counter * MPI_CHUNK_SIZE
        perf_diffs.resize(current_size)
        

if __name__ == "__main__":
    sys.exit(main())

