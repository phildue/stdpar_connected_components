# Parallel 8-way Connected Component Labelling

An 8-way implementation of the Playne-equvalence algorithm for connected component labelling using C++ standard parallelism.
Based on the illustrative example by Daniel Playne: <https://github.com/DanielPlayne/playne-equivalence-algorithm> as originally described in:

D. P. Playne and K. Hawick,
"A New Algorithm for Parallel Connected-Component Labelling on GPUs,"
in IEEE Transactions on Parallel and Distributed Systems,
vol. 29, no. 6, pp. 1217-1230, 1 June 2018.

* URL: <https://ieeexplore.ieee.org/document/8274991>

This code has mostly been ported from [Folke Vesterlund](https://github.com/FolkeV/CUDA_CCL).

## Prerequisites

* `OpenCV` is used to load and display images, it is assumed that it has been installed correctly.
* `NVHPC` is used for compilation

The development container can be setup with the [Dockerfile](.devcontainer/Dockerfile)

## Build

```bash

mkdir build
cd build
cmake ..
make

```

## Usage

`$ ./<main> <image-file>`

## License

The source code is provided under The MIT license
