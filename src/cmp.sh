#!/bin/bash
#
# License: CC0
#

gcc -O3 hn-parallel-item.c -lcurl -o hnai
gcc -g hn-parallel-user.c -lcurl -o hnau
