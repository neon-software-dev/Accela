#!/bin/sh
for f in $(find ../../../ -type f \( -iname \*.h -o -iname \*.cpp -o -iname \*.vert -o -iname \*.frag -o -iname \*.tesc -o -iname \*.tese \) -not -path "../../../src/External/*"); do
    cat ./file_license_disclaimer.txt $f >> $f.$$
    mv $f.$$ $f
done
