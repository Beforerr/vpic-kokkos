home_dir := env_var('HOME')
vpic := home_dir + "/src/vpic-kokkos/build/bin/vpic"

compile:
  cd {{invocation_directory()}}; {{vpic}} *.cxx

install:
  #!/usr/bin/env bash
  # git clone https://github.com/lanl/vpic-kokkos.git
  module swap netcdf netcdf-mpi
  module load cmake

  git submodule update --init
  git checkout hybridVPIC
  mkdir build
  cd build
  cmake ..
  make -j 8

clean:
  find . -name '.DS_Store' -type f -delete