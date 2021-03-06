#!/bin/bash -x

# Download the requested version of Eigen
mkdir -p eigen
cd eigen
wget --quiet "http://bitbucket.org/eigen/eigen/get/${EIGEN_VERSION}.tar.gz"
tar -xf ${EIGEN_VERSION}.tar.gz --strip-components 1
cd ..

# If building the paper, do that here
if [[ $TEST_LANG == paper ]]
then
  if git diff --name-only $TRAVIS_COMMIT_RANGE | grep 'paper/'
  then
    echo "Building the paper..."
    export GENRP_BUILDING_PAPER=true
    source "$( dirname "${BASH_SOURCE[0]}" )"/setup-texlive.sh
    return
  fi
  export GENRP_BUILDING_PAPER=false
  return
fi

# If testing C++, deal with that here
if [[ $TEST_LANG == cpp ]]
then
  cd cpp
  cmake . -DEIGEN_CHECK_INCLUDE_DIRS=../eigen
  make
  cd ..
  return
fi

# Conda Python
hash -r
conda config --set always_yes yes --set changeps1 no
conda update -q conda
conda info -a
conda create --yes -n test python=$PYTHON_VERSION numpy=$NUMPY_VERSION Cython setuptools pytest
source activate test

# Build the extension
cd python
CXX=g++ python setup.py build_ext -I../eigen --inplace
cd ..
