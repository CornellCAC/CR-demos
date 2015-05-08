#!/bin/sh

#SBATCH -J myMPI          # Job Name
#SBATCH -o myMPI.o%j      # Name of the output file (myMPI.oJobID)
#SBATCH -p development    # Queue name
#SBATCH -t 00:00:08       # Run time (hh:mm:ss) - 8 seconds
#SBATCH -n 20             # 4 tasks total
#SBATCH -e myMPI.err%j    # Direct error to the error file
#SBATCH -A TG-STA110019S  # Account number (replace with yours)

ibrun dmtcp_launch -i 4 ./mpi_count
