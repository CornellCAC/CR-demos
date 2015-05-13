## Setting up the environment

First set up the environment so that DMTCP executables are
in your path. If you've loaded a DMTCP module this won't
be necessary (but it will be necessary to load the
dmtcp module before submitting a job):

```
export DMTCP_ROOT=$WORK/dmtcp
export PATH=$PATH:$DMTCP_ROOT/bin
```

Making sure that version 2.1 or greater of MVAPICH2 is loaded as well,
now you can compile the MPI counting demo. Very likely you should be
using the Intel compiler with version of at least 15.0.2: `module load
intel/15.0.2`.  Note that this will pull in a different build of
MVAPICH2, and should also be at least version 2.1 (as with gcc above).

```
Lmod is automatically replacing "gcc/4.9.1" with "intel/15.0.2"


Due to MODULEPATH changes the following have been reloaded:
  1) mvapich2/2.1
```

Now let's build an MPI demo:

```
cd CR-demos/demos/MPI/
mpicc mpi_count.c -o mpi_count
```

## Launching a job with DMTCP

Next comes the interesting part: submitting a job under
the control of dmtcp. You are encouraged to check the 
`mpi_count_with_dmtcp.sh` script (especially if using a system other than
Stampede), which is itself based on the 
example found in [slurm_launch.job]
(https://github.com/dmtcp/dmtcp/blob/master/plugin/batch-queue/job_examples/slurm_launch.job) 
of the DMTCP repository, which you may want to
check for possible updates.

Briefly, this script does the following in addittion
to what a normal sbatch script does:

1. Runs `dmtcp_coordinator` with instructions to checkpiont every ten
seconds, which is much more frequently than you'd want in a production
script (due to checkpoint overhead).  
2. Simultaneously, `dmtcp_coordinator` is run with an automatically
allocated port, to save you the trouble of finding an open port.
3. Creates a job-specific `dmtcp_command.$JOBID` script to allow you
to interact with the job without having to pass port and hostname
parameters to `dmtcp_command`.
4. Calls ibrun (the Stampede job runner) on a
dmtcp_launched mpi_counter: `ibrun dmtcp_launch ./mpi_count `.

Once you've adjusted your script, simply run the script as usual:

```sh
sbatch mpi_count_with_dmtcp.sh
```


## Resuming a job with DMTCP

Now you may be wondering, after seeing our previous examples, how the
restart process works, since as we've seen a different dmtcp command
is used for restaring that launching. Indeed, a separate sbatch script
is needed in this case.

to be continued ...

If you wish to change the DMTCP parameters for the restart, you can
run `dmtcp_restart -h` to see valid parameters for your installed
version of DMTCP.

Also, once launched, you can see how to interact with your running job
(e.g. you can manually checkpoint it) using `dmtcp_command -h`, but
you should use the job-specific command script created by the sbatch
script to actually run these commands.
