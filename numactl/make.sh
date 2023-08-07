#!/bin/bash

for f in *c ; do
	echo "gcc -Wall -c $f"
	gcc -Wall -c $f
done

echo $gcc -o memhog util.o syscall.o libnuma.o affinity.o sysfs.o rtnetlink.o memhog.o$
gcc -o memhog util.o syscall.o libnuma.o affinity.o sysfs.o rtnetlink.o memhog.o
