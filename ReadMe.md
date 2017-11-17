# Crystal-MD

A molecular dynamics (MD) simulation program.  
Author:[Baihe](mailto:baihe_ustb@163.com)
Update:[Chugenshen](mailto:genshenchu@gmail.com)

### build&run
#### build from CMake (recommend)  
dependency:
1. cmake CMake 3.6 or higher is required;
2. c++ 11 feature required(check your gcc version);
3. mpi.

```sh
mkdir build
cd build
cmake ../
make
cd src  # executable file is located in src directory.
./Crystal-MD  --help # run for showing help.
mpiexec -n 64 ./Crystal-MD -c config.toml  # run MD simulation
```

#### build from Makefile
```sh
cd src
mkdir build
make
cp ./Crystal-MD ../example/  # copy executable file to example directory.
cd ../example
mpiexec -n 100 ./Crystal-MD

```
