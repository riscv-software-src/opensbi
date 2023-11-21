#!/bin/bash

cd /app/
export OPENSBI_CC_ABI=lp64d
export CROSS_COMPILE=riscv64-linux-gnu- 
make -j4 PLATFORM=generic FW_PAYLOAD=y FW_PAYLOAD_PATH=boot_loader.bin FW_PAYLOAD_OFFSET=0x200000

