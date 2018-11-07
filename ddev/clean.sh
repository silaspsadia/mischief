#!/bin/sh

for D in */
do
	cd $D && make oclean
	cd ..
done
