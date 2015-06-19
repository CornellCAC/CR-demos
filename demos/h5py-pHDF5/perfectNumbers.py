#!/usr/bin/python

#TODO: Remove MPI from this to make it as simple as possible for
#first version

import sys

import h5py
import numpy as np
from mpi4py import MPI

#TODO: figure out how to block system calls?

H5FILE_NAME    = "perfectNumbers.h5"
H5FILE_BACKUP  = "perfectNumbers.h5.bak"
BACKUP_CMD     = "/bin/cp " + H5FILE_NAME + " " + H5FILE_BACKUP
DATASETNAME    = "DifferenceFromPerfect"
STATUSGROUP    = "status"
RANK           = 1   

MPI_CHUNK_SIZE = 100

#
# MPI variables
#

comm = MPI.COMM_WORLD

# Tells if a given integer is a perfect number by returning the
# difference between itself and the sum of its
# divisors (excluding itself); if the diff is 0, it is perfect.
# 
# This is toy code. Odd perfect numbers are currently being searched
# for above 10^300: http://www.oddperfect.org
# Also see : http://rosettacode.org/wiki/Perfect_numbers
#          : http://en.wikipedia.org/wiki/List_of_perfect_numbers
def perfect_diff(n):
    divisor_sum = 0
    for i in range(1, n): 
        if n % i == 0:
            divisor_sum += i
    return divisor_sum - n

def broadcast_state():
    return True


def main(argv=None):
    if argv is None:
        argv = sys.argv

    mpi_rank = comm.Get_rank()
    mpi_size = comm.Get_size()

    num_even = 0
    num_odd = 0
    chunk_counter = 0

    #If not restored (TODO)
    counter = MPI_CHUNK_SIZE * mpi_rank
    chunk_counter += 1
    current_size = chunk_counter * MPI_CHUNK_SIZE    
    perf_diffs = np.zeros(current_size, dtype=int)

    while True:
        new_evens = new_odds = 0
        while True:
            index = MPI_CHUNK_SIZE * (chunk_counter-1) + counter % MPI_CHUNK_SIZE
            perf_diffs[index] = perfect_diff(counter) 
            # perf_diffs[index] = counter # For DEBUG
            if (perf_diffs[index] == 0):
                print("Found %d!\n" % counter)
                if counter % 2 == 0:
                    new_evens += 1
                else:
                    new_odds += 1
            counter += 1
            if counter % MPI_CHUNK_SIZE == 0:
                break
        broadcast_state()
        num_even += new_evens
        num_odd  += new_odds
        # checkpoint(comm, info, perf_diffs)
        # offset into next iteration
        counter += (mpi_size - 1) * MPI_CHUNK_SIZE
        chunk_counter += 1
        current_size = chunk_counter * MPI_CHUNK_SIZE
        perf_diffs.resize(current_size)
        

if __name__ == "__main__":
    sys.exit(main())

