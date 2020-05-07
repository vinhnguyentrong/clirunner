#!/bin/bash

echo "======= remove to build again ========"

echo make clean
make clean

echo $(rm -r CMakeFiles)
echo rm CMakeCache.txt cmake_install.cmake CPackConfig.cmake CPackSourceConfig.cmake install_manifest.txt Makefile
rm CMakeCache.txt cmake_install.cmake CPackConfig.cmake CPackSourceConfig.cmake install_manifest.txt Makefile

echo rm -r lib/CMakeFiles
rm -r lib/CMakeFiles

echo rm lib/cmake_install.cmake lib/Makefile
rm ./lib/cmake_install.cmake ./lib/Makefile

echo rm -r vtysh/CMakeFiles cmake_install.cmake Makefile
rm -r vtysh/CMakeFiles 
rm ./vtysh/cmake_install.cmake ./vtysh/Makefile

rm -r ./dependencies/pam_tacplus-1.4.1/

