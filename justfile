prequisites:
  spack install mpich

install:
  #!/usr/bin/env bash
  # git clone https://github.com/lanl/vpic-kokkos.git
  git submodule update --init
  git checkout hybridVPIC
  mkdir build
  cd build
  cmake ..
  make -j