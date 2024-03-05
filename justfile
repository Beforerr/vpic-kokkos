home_dir := env_var('HOME')
vpic := home_dir + "/projects/vpic-kokkos/build/bin/vpic"

compile:
    {{vpic}} *.cxx

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

clean:
  find . -name '.DS_Store' -type f -delete