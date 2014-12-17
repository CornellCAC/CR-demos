## Building DMTCP on Stampede

To build, you can download DMTCP (we used the latest version at the 
time of this writing, 2.3.1).

You can build it with Infiniband support. There are several Infiniband
installations; for the moment, we want the one that is apparently
x86_64 and not MIC. This installation is in `/opt/ofed`.

Before running the configure script, we need to change a variable that
will influence tests on Stampede; comment out the existing LIMIT assignment
in `Makefile.in` and change it to something benign, like so:

```
#LIMIT=ulimit -v 33554432
LIMIT=echo "No ulimit -v"

```

Now, we need to be explicit with where to find our MPICH variant.
This will vary depending on which MPI module you have loaded, and
not all may work. There are a number of MPI executables that are 
looked for by the configure script (see configure.ac for details).
Unfortunately, in some environment modules, these may be located
in different directories. With mvapich2-x/2.0b loaded, the following
should work:

```
login2.stampede(10)$ mkdir ~/mpich
login2.stampede(11)$ ln -s /opt/apps/intel/13/composer_xe_2013.2.146/mpirt/bin/intel64/* ~/mpich/
login2.stampede(12)$ ln -s -f /home1/apps/intel13/mvapich2-x/2.0b/bin/* ~/mpich/

```
This puts symbolic links into one directory to make it possible
for the existing configure script to find all MPICH executables.

Now we are ready to configure and build (the mpich home should be adjusted appropriately):

```
LDFLAGS=-L/opt/ofed/lib64 CFLAGS=-I/opt/ofed/include ./configure --enable-infiniband-support --with-mpich=/home1/03135/bebarker/mpich
make
```

It is a good idea to run `make check`, but due to the Stampede environment being slightly different than the typical desktop environment, it should be run as follows:

```
make check HAS_JAVA=no HAS_JAVAC=no
```


## Using DMTCP

Make sure the newly built DMTCP is in your path, e.g.:

```
export PATH=$PATH:$WORK/dmtcp-2.3.1/bin
```

Now you can proceed to run examples as described elsewhere in the docs.


