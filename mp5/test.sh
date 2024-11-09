#!/bin/bash

make
./copykernel.sh
bochs -f bochsrc.bxrc