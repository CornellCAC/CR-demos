#!/usr/bin/python

import os
import signal
import sys

import h5py
import numpy as np
from mpi4py import MPI

H5FILE_NAME    = "perfectNumbers.h5"
H5FILE_BACKUP  = "perfectNumbers.h5.bak"
BACKUP_CMD     = "/bin/cp " + H5FILE_NAME + " " + H5FILE_BACKUP
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
sig_exit      = False

#
# MPI variables
#

comm = MPI.COMM_WORLD
info = MPI.INFO_NULL
mpi_rank = comm.Get_rank()
mpi_size = comm.Get_size()

def perfect_diff(n):
    divisor_sum = 0
    for i in range(1, n): 
        if n % i == 0:
            divisor_sum += i
    return divisor_sum - n

def broadcast_state(comm, new_evens, new_odds):
    mpi_type = MPI.__TypeDict__[new_evens.dtype.char]
    comm.Allreduce(MPI.IN_PLACE, [new_evens, mpi_type], op=MPI.SUM)
    comm.Allreduce(MPI.IN_PLACE, [new_odds,  mpi_type], op=MPI.SUM)
    return 0

def backup_file():
    os.system(BACKUP_CMD)

def sig_safe_handler(signal, frame):
    global sig_exit
    sig_exit = True

#
# Calculates the number of chunks to allocate
# upon a restore
#
def get_restore_chunk_counter(datasize):
    total_chunks = datasize / MPI_CHUNK_SIZE
    if ((datasize % MPI_CHUNK_SIZE) != 0):
        total_chunks += 1
    if ((total_chunks % mpi_size) != 0):
        return total_chunks / mpi_size + 1
    else:
        return total_chunks / mpi_size

def checkpoint(comm, info):
    dimsm  = (chunk_counter * MPI_CHUNK_SIZE,)
    dimsf  = (dimsm[0] * mpi_size,)

    start  = mpi_rank * MPI_CHUNK_SIZE
    end    = start + MPI_CHUNK_SIZE

    signal.signal(signal.SIGINT,  sig_safe_handler)
    signal.signal(signal.SIGTERM, sig_safe_handler)

    if mpi_rank == 0:
        if os.path.isfile(H5FILE_NAME):
            backup_file()

    file_id = h5py.File(H5FILE_NAME, "w", driver="mpio", comm=comm)
    dset_id = file_id.create_dataset(DATASETNAME, shape=dimsf, dtype='i8')

    status_id = file_id.create_group(STATUSGROUP)
    status_id.attrs[NUM_EVEN_ATTR] = num_even
    status_id.attrs[NUM_ODD_ATTR]  = num_odd

    for ii in range(0, chunk_counter):
        start_ii = start + ii*mpi_size*MPI_CHUNK_SIZE
        end_ii   = start_ii + MPI_CHUNK_SIZE
        dset_id[start_ii:end_ii] = perf_diffs[ii*MPI_CHUNK_SIZE:(ii+1)*MPI_CHUNK_SIZE]
    
    signal.signal(signal.SIGINT,  signal.SIG_DFL)
    signal.signal(signal.SIGTERM, signal.SIG_DFL)
    if sig_exit:
        file_id.flush()
        file_id.close()
        sys.exit(0)
    file_id.close()

def restore(comm, info):
    global chunk_counter
    global counter
    global current_size
    global num_even
    global num_odd
    global perf_diffs

    start = mpi_rank * MPI_CHUNK_SIZE

    file_id = h5py.File(H5FILE_NAME, "r", driver="mpio", comm=comm)    
    dset_id = file_id[DATASETNAME]
    dimsf = dset_id.shape
    chunk_counter = get_restore_chunk_counter(dimsf[0])
    dimsm = (chunk_counter * MPI_CHUNK_SIZE,)
    counter = ((chunk_counter - 1) * mpi_size + mpi_rank) * MPI_CHUNK_SIZE
    current_size = dimsm[0]
    perf_diffs.resize(current_size)

    for ii in range(0, chunk_counter):
        start_ii = start + ii*mpi_size*MPI_CHUNK_SIZE
        end_ii   = start_ii + MPI_CHUNK_SIZE
        perf_diffs[ii*MPI_CHUNK_SIZE:(ii+1)*MPI_CHUNK_SIZE] = dset_id[start_ii:end_ii]
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
        restore(comm, info)
    else:
        counter = MPI_CHUNK_SIZE * mpi_rank
        chunk_counter += 1
        current_size  = chunk_counter * MPI_CHUNK_SIZE
        perf_diffs.resize(current_size)

    while True:
        new_evens = np.zeros(1, dtype=int)
        new_odds = np.zeros(1, dtype=int)
        while True:
            index = MPI_CHUNK_SIZE * (chunk_counter-1) + counter % MPI_CHUNK_SIZE
            perf_diffs[index] = perfect_diff(counter) 
            if (perf_diffs[index] == 0):
                print("Found %d!\n" % counter)
                if counter % 2 == 0:
                    new_evens[0] += 1
                else:
                    new_odds[0] += 1
            perf_diffs[index] = counter # For DEBUG
            counter += 1
            if counter % MPI_CHUNK_SIZE == 0:
                break
        broadcast_state(comm, new_evens, new_odds)
        num_even += new_evens[0]
        num_odd  += new_odds[0]
        checkpoint(comm, info)
        # offset into next iteration
        counter += (mpi_size - 1) * MPI_CHUNK_SIZE
        chunk_counter += 1
        current_size = chunk_counter * MPI_CHUNK_SIZE
        perf_diffs.resize(current_size)
        

if __name__ == "__main__":
    sys.exit(main())

