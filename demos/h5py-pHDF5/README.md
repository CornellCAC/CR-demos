After making sure the appropriate (p)HDF5 and MPI libraries are
installed and available in your environment, you will also need to have
mpi4py and h5py installed as well as their dependencies. On your own system,
you can try handle the python packages with `pip`:


```
pip install h5py
pip install mpi4py
```

If your system doesn't have `pip`, consider installing it with
easy_install, which is included with most Linux systems. Otherwise,
you can try directly installing these modules with easy_install.

Unfortunately pip isn't always flexible, and on Stampede and other large
HPC systems, this simple recipe almost certainly won't work due to
the presence of multiple versions of almost every library and compiler.
In particular, `pip install h5py` is unlikely to work, since the default
configuration won't enable MPI support through mpi4py. If you aren't worried about mpi4py
support just yet, you can happily avoid this concern for now. If you do need it, you'll
probably need to manually download and install the h5py archive to get things
up and running quickly. If this describes your situation, or the above 
didn't work for you, please see the [installation notes](#installation-notes) below.

We also note each system is different, and for the most performant installation of
these libraries you'll likely benefit from reading the [Compiling for
Performance](https://www.cac.cornell.edu/VW/python/compiling.aspx)
section of the [Python for High
Performance](https://www.cac.cornell.edu/VW/python/) module.  For
development purposes and to get going, the instructions on this page should
be fine.


## Installation Notes

If your system doesn't have some of the required packages, you may
need to install them. Python module dependencies should be pulled in
automatically by pip and other python module managers, but system
libraries like python headers, MPI, and HDF5 will not. In particular
for headers of relevant packages, you may need to install packages
named like libhdf5-dev, python-dev, mpich2, libhdf5-mpich2-dev etc.
These should already be installed on Stampede, but you'll need
to make them accessible to your environemnt:

```
module load intel/15.0.2
module load python/2.7.9
module load phdf5/1.8.14
```

Note that module-loading the Intel 15 compilers should also load a suitable
version of MVAPICH2. It is possible that this document may be out of date, 
so you may want to experiment with the module versions above.

Create a new virual environment for your HDF5 work 
(you can choose what to name it and where to put it):

```
virtualenv ~/virtualenv/phdf5-intel15
```

Now, activate your new virtual environment:

```
source ~/virtualenv/phdf5-intel15/bin/activate
```

At this point, you can try to run the perfectNumbers.py script, but it will be unlikely to work.
Still, the errors should be instructive. It is important that you use the `python`
command directly so that thepython interpreter in your current virtual environment is used.
Now let's try running it:

```
cd CR-demos/demos/h5py-pHDF5
python perfectNumbers.py
```


If you have a library loading problem, you may need to manually correct the library path. For instance,
the following error tells us python can't find the hdf5 library: 

```
FIXME
```

Let's add it to our environment's path:

```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/apps/intel15/mvapich2_2_1/phdf5/1.8.14/x86_64/lib
```

Try running it again. If you get an error that looks like the following, with the salient
feature being that h5py files don't know about the name 'mpi4py', it means that you your
installation of h5py has unfortunately not been configured to use mpi4py, and it is time to get
your own installation:

```
Traceback (most recent call last):
  File "perfectNumbers.py", line 186, in <module>
    sys.exit(main())
  File "perfectNumbers.py", line 177, in main
    checkpoint(comm, info)
  File "perfectNumbers.py", line 90, in checkpoint
    file_id = h5py.File(H5FILE_NAME, "w", driver="mpio", comm=comm)
  File "/opt/apps/intel15/python/2.7.9/lib/python2.7/site-packages/h5py/_hl/files.py", line 259, in __init__
    fapl = make_fapl(driver, libver, **kwds)
  File "/opt/apps/intel15/python/2.7.9/lib/python2.7/site-packages/h5py/_hl/files.py", line 61, in make_fapl
    kwds.setdefault('info', mpi4py.MPI.Info())
NameError: global name 'mpi4py' is not defined
```

You may be able to do this, with pip, but in our experience this won't enable mpi4py support:

```
pip install --user -I h5py # Don't bet on this working
```

Instead, you may want to [download a recent version](https://pypi.python.org/pypi/h5py) as a tarball. From the command line, you can use `wget`:

```
wget https://pypi.python.org/packages/source/h/h5py/h5py-2.5.0.tar.gz#md5=6e4301b5ad5da0d51b0a1e5ac19e3b74
tar zxvf h5py-2.5.0.tar.gz
```

Once extracted, just do:

```
cd h5py-2.5.0
export CC=mpicc
python setup.py configure --mpi
python setup.py install
```


Similar to the aforementioned library loading error, we may also find an issue with finding C headers or libraries for linking:

```
 catastrophic error: cannot open source file "hdf5.h"
```

or

```
/usr/bin/ld: cannot find -lhdf5 
 ```
 
 Doing the following should take care of the issue, but again, make sure the paths listed
 below match your compiler and MPI implementations that are loaded in your environment:

```
export CFLAGS="$CFLAGS -I/opt/apps/intel15/mvapich2_2_1/phdf5/1.8.14/x86_64/include"
export CPPFLAGS="$CPPFLAGS -I/opt/apps/intel15/mvapich2_2_1/phdf5/1.8.14/x86_64/include"
export "LDFLAGS=$LDFLAGS -L/opt/apps/intel15/mvapich2_2_1/phdf5/1.8.14/x86_64/lib"
```

Before running again, we want to make absolutely sure we are using our local libraries,
particularly h5py, so let's adjust `PYTHONPATH` so it sees our local packages *first*:

```
export PYTHONPATH=$HOME/virtualenv/phdf5-intel15/lib/python2.7/site-packages:$PYTHONPATH
```

Now try to run `python setup.py install` and you should hopefully have a working h5py installation! 
Try running the `perfectNumbers.py` script again.
How to deal with Python modules and libraries on an HPC system is 
a bit of a moving target, so if the above doesn't work, you may want to consider consulting
documentation, or failing that, your systems administrator.

