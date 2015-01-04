#!/bin/sh

#SBATCH -J myMPI          # Job Name
#SBATCH -o myMPI.o%j      # Name of the output file (myMPI.oJobID)
#SBATCH -p development    # Queue name
#SBATCH -t 00:00:15       # Run time (hh:mm:ss) - 15 seconds
#SBATCH -n 4              # 4 tasks total
#SBATCH -e myMPI.err%j    # Direct error to the error file
#SBATCH -A TG-STA110019S  # Account number (replace with yours)

ibrun mpi_count
