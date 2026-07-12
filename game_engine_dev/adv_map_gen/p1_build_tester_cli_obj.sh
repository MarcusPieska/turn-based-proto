#!/bin/bash

if [ -n "$P1_TESTER_CLI_BUILT" ]; then
    return 0 2>/dev/null || exit 0
fi

CLI_INC="${P1_TESTER_CLI_INC:--I. -I..}"
g++ -std=c++11 $CLI_INC -c p1_tester_cli.cpp -o p1_tester_cli.o
P1_TESTER_CLI_OBJ=p1_tester_cli.o
export P1_TESTER_CLI_OBJ
export P1_TESTER_CLI_BUILT=1
