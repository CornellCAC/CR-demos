#!/bin/sh
#Stampede:
module load intel/15.0.2
module load phdf5/1.8.14
mpicc -o perfectNumbers -I$TACC_HDF5_INC -Wl,-rpath,$TACC_HDF5_LIB -L$TACC_HDF5_LIB -lhdf5 -lz perfectNumbers.c