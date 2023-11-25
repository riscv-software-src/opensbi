#!/bin/bash

export OPENSBI_CC_ABI=lp64d
export CROSS_COMPILE=riscv64-linux-gnu- 
make -j4

