#!/usr/bin/python

import os
import sys

import h5py
import numpy as np

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
    divisor_sum = 0
    for i in range(1, n): 
        if n % i == 0:
            divisor_sum += i
    return divisor_sum - n

def checkpoint():
    dimsf  = (chunk_counter * MPI_CHUNK_SIZE,)

    file_id = h5py.File(H5FILE_NAME, "w")
    dset_id = file_id.create_dataset(DATASETNAME, shape=dimsf, dtype='i8')

    status_id = file_id.create_group(STATUSGROUP)
    status_id.attrs[NUM_EVEN_ATTR] = num_even
    status_id.attrs[NUM_ODD_ATTR]  = num_odd

    dset_id[...] = perf_diffs
    
    file_id.close()

def restore():
    global chunk_counter
    global counter
    global current_size
    global num_even
    global num_odd
    global perf_diffs

    file_id = h5py.File(H5FILE_NAME, "r")    
    dset_id = file_id[DATASETNAME]
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

    while True:
        while True:
            index = MPI_CHUNK_SIZE * (chunk_counter-1) + counter % MPI_CHUNK_SIZE
            perf_diffs[index] = perfect_diff(counter) 
            if (perf_diffs[index] == 0):
                print("Found %d!\n" % counter)
                if counter % 2 == 0:
                    num_even += 1
                else:
                    num_odd += 1
            perf_diffs[index] = counter # For DEBUG
            counter += 1
            if counter % MPI_CHUNK_SIZE == 0:
                break
        checkpoint()
        chunk_counter += 1
        current_size = chunk_counter * MPI_CHUNK_SIZE
        perf_diffs.resize(current_size)
        

if __name__ == "__main__":
    sys.exit(main())

