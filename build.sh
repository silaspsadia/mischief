#!/bin/sh

for D in */
do
	cd $D && make
	cd ..
done
