#!/bin/bash

CMAKE="cmake CMakeLists.txt -Wno-dev"

function cmake_distclean()
{
	for fld in $(find -name "CMakeLists.txt" -printf '%h ')
	do
		for cmakefile in CMakeCache.txt cmake_install.cmake CTestTestfile.cmake CMakeFiles Makefile install_manifest.txt
			do
				rm -rf $fld/$cmakefile
			done
	done
}

cmake_distclean && $CMAKE

exit 0
