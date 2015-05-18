#!/bin/sh
#Stampede:
mpicc -o perfectNumbers -I$TACC_HDF5_INC -Wl,-rpath,$TACC_HDF5_LIB -L$TACC_HDF5_LIB -lhdf5 -lz perfectNumbers.c