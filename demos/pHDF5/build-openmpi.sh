#!/bin/sh
HDF5_LIB=/usr/lib64/openmpi/lib
# -I$TACC_HDF5_INC -Wl,-rpath,$TACC_HDF5_LIB -L$TACC_HDF5_LIB
mpicc -o perfectNumbers -Wl,-rpath,$HDF5_LIB -L$HDF5_LIB -lhdf5 -lz perfectNumbers.c
