#!/bin/bash

P=`pwd`
user=`id -u`

docker run --user $user:$user -v $P:/app -it --rm pafos/releng:main /app/cross_build.sh


