#!/bin/bash

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")
cd $BASEDIR

if [ ! -d cdn ] || [ ! -f readme.txt ]
then
    echo "ERROR: $BASEDIR is not a valid directory of SDK-gcc for cdn."
    echo "  Please run this script in a regular directory of SDK-gcc."
    exit -1
fi

tmp=$(cmake --version 2>&1)
if [ $? -ne 0 ]
then
    echo "ERROR: You should install cmake(2.8 or later) first."
    echo "  Please run 'sudo apt-get install cmake' to install it."
    echo "  or goto https://cmake.org to download and install it."
    exit
fi

rm -fr bin
mkdir bin
#rm -fr build
mkdir build
cd build
cmake ../cdn
make

cd ..
tar -zcPf cdn.tar.gz *
# bin/cdn ./case_example/topo.txt ./case_example/result.txt
# bin/cdn ./case_example/case0.txt ./case_example/result.txt
#bin/cdn ./case_example/case1.txt ./case_example/result.txt
#bin/cdn ./case_example/case2.txt ./case_example/result.txt
#bin/cdn ./case_example/case3.txt ./case_example/result.txt
#bin/cdn ./case_example/case4.txt ./case_example/result.txt
# bin/cdn ./case_example/case0.txt ./result.txt
# bin/cdn ./case_example/case11.txt ./result.txt

#simple case
# bin/cdn ./new_case/0/case0.txt ./result.txt
#bin/cdn ./new_case/0/1.txt ./result.txt
#bin/cdn ./new_case/0/2.txt ./result.txt
#bin/cdn ./new_case/0/3.txt ./result.txt
#bin/cdn ./new_case/0/4.txt ./result.txt
#bin/cdn ./new_case/0/5.txt ./result.txt
#bin/cdn ./new_case/0/6.txt ./result.txt
#bin/cdn ./new_case/0/7.txt ./result.txt
#bin/cdn ./new_case/0/8.txt ./result.txt
#
##high case
#bin/cdn ./new_case/1/case0.txt ./result.txt
# bin/cdn ./new_case/1/case0.txt ./result.txt
# bin/cdn ./new_case/1/case0.txt ./result.txt
# bin/cdn ./new_case/1/case0.txt ./result.txt
# bin/cdn ./new_case/1/case0.txt ./result.txt
# bin/cdn ./new_case/1/case0.txt ./result.txt
# bin/cdn ./new_case/1/case0.txt ./result.txt
# bin/cdn ./new_case/1/case0.txt ./result.txt
# bin/cdn ./new_case/1/case0.txt ./result.txt
# bin/cdn ./new_case/1/case0.txt ./result.txt
# bin/cdn ./new_case/1/case0.txt ./result.txt
#bin/cdn ./new_case/1/1.txt ./result.txt
#bin/cdn ./new_case/1/2.txt ./result.txt
#bin/cdn ./new_case/1/3.txt ./result.txt
#bin/cdn ./new_case/1/4.txt ./result.txt
#bin/cdn ./new_case/1/5.txt ./result.txt
#bin/cdn ./new_case/1/6.txt ./result.txt
#bin/cdn ./new_case/1/7.txt ./result.txt
#bin/cdn ./new_case/1/8.txt ./result.txt
#
##very high case
# bin/cdn ./new_case/2/case0.txt ./result.txt
# bin/cdn ./new_case/2/case1.txt ./result.txt
#bin/cdn ./new_case/2/2.txt ./result.txt
#bin/cdn ./new_case/2/3.txt ./result.txt
#bin/cdn ./new_case/2/4.txt ./result.txt
#bin/cdn ./new_case/2/5.txt ./result.txt
#bin/cdn ./new_case/2/6.txt ./result.txt
#bin/cdn ./new_case/2/7.txt ./result.txt
#bin/cdn ./new_case/2/8.txt ./result.txt
#