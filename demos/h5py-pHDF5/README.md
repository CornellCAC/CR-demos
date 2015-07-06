After making sure the appropriate (p)HDF5 and MPI libraries are
installed (see Installation Notes below) and available in your
environment, you can instally h5py (if it is not already available):

Using pip:

```
pip install h5py
pip install mpi4py
```

If your system doesn't have pip, consider installing it with
easy_install, which is included with most Linux systems. Otherwise,
you can try directly installing these modules with easy_install.





## Installation Notes

If your system doesn't have some of the required packages, you may
need to install them. Python module dependencies should be pulled in
automatically by pip and other python module managers, but system
libraries like python headers, MPI, and HDF5 will not. In particular
for headers of relevant packages, you may need to install packages
named like libhdf5-dev, python-dev, mpich2, libhdf5-mpich2-dev etc.

Each system is different, and for the most performant installation of
these libraries you'll likely benefit from reading the [Compiling for
Performance](https://www.cac.cornell.edu/VW/python/compiling.aspx)
section of the [Python for High
Performance](https://www.cac.cornell.edu/VW/python/) module.  For
development purposes and to get going, the above instructions should
be fine.

In some cases, `pip install h5py` will not work, since it won't
be configured to use MPI (or more precisely, mpi4py). You can
download the h5py archive manually and install to get things
up and running quickly. Once extracted, just do:

```
python setup.py configure --mpi
echo "export CC=mpicc; python setup.py install" | sudo sh
```
