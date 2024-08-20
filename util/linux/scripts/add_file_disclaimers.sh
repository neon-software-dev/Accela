#!/bin/sh
any_changed=0

for f in $(find ../../../ -type f \( -iname \*.h -o -iname \*.cpp -o -iname \*.vert -o -iname \*.frag -o -iname \*.tesc -o -iname \*.tese \) -not -path "../../../external/*" -not -path "../../../src/LibAccelaRendererVk/src/SPIRV/*"); do
    if  grep -q "SPDX-License-Identifier" "$f" ; then
        continue;
    fi
    echo "Adding copyright disclaimer to: $f" ; 
    cat ./file_license_disclaimer.txt $f >> $f.$$
    mv $f.$$ $f
    
    any_changed=1
done

exit $any_changed
