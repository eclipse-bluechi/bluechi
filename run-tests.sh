#!/bin/bash

array=( client common node orch )
for i in "${array[@]}"
do
    tests=$(ls ./test/$i/*_test.c 2>/dev/null)

    test_binaries=()
    for t in $tests
    do
        n=${t::len-2}
        # extend this by additional c files as tests grow
        gcc -D_GNU_SOURCE $t -g -O1 -Wall -o $n
        test_binaries+=($n)
    done

    for t in ${test_binaries[@]}
    do
        echo "Running $t..."
        RED='\033[0;31m'
        NC='\033[0m'
        printf "${RED}"
        ./$t
        printf "${NC}"
        rm $t
    done

done
