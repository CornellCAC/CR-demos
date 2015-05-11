#!/bin/sh

#SBATCH -J myMPI          # Job Name
#SBATCH -o myMPI.o%j      # Name of the output file (myMPI.oJobID)
#SBATCH -p development    # Queue name
#SBATCH -t 00:00:45       # Run time (hh:mm:ss) - 45 seconds
#SBATCH -n 4              # 4 tasks total
#SBATCH -N 2              # run on 2 nodes
#SBATCH -e myMPI.err%j    # Direct error to the error file
#SBATCH -A TG-STA110019S  # Account number (replace with yours)

ibrun ./mpi_count
