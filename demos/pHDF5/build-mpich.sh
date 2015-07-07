#!/bin/sh
#
#Not working currently!
#

HDF5_LIB=/usr/lib/x86_64-linux-gnu
HDF5_INC=/usr/include
# -I$TACC_HDF5_INC -Wl,-rpath,$TACC_HDF5_LIB -L$TACC_HDF5_LIB
#mpicc -g -O0 -o perfectNumbers -Wl,-rpath,$HDF5_LIB -L$HDF5_LIB -lhdf5 -lz perfectNumbers.c
mpicc -g -O0 -o perfectNumbers -I$HDF5_INC-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_BSD_SOURCE -D_FORTIFY_SOURCE=2 -g -O2 -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security -L$HDF5_LIB $HDF5_LIB/libhdf5_hl.a $HDF5_LIB/libhdf5.a -Wl,-Bsymbolic-functions -Wl,-z,relro -lpthread -lz -ldl -lm -Wl,-rpath -Wl,$HDF5_LIB -lhdf5 -lz perfectNumbers.c
