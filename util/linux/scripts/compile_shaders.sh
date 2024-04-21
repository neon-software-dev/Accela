#!/bin/sh

cd ../../../src/TestDesktopApp/assets/shaders/

for file in ./*.glsl; do
 [ -f "$file" ] || continue
 echo "Compiling $file..."
 ../../../../util/linux/glslc --target-env=vulkan1.2 $file -o $file.spv
done

for file in ./*.vert; do
 [ -f "$file" ] || continue
 echo "Compiling $file..."
 ../../../../util/linux/glslc --target-env=vulkan1.2 $file -o $file.spv
done

for file in ./*.frag; do
 [ -f "$file" ] || continue
 echo "Compiling $file..."
 ../../../../util/linux/glslc --target-env=vulkan1.2 $file -o $file.spv
done

for file in ./*.tesc; do
 [ -f "$file" ] || continue
 echo "Compiling $file..."
 ../../../../util/linux/glslc --target-env=vulkan1.2 $file -o $file.spv 
done

for file in ./*.tese; do
 [ -f "$file" ] || continue
 echo "Compiling $file..."
 ../../../../util/linux/glslc --target-env=vulkan1.2 $file -o $file.spv 
done
