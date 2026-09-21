#!/bin/sh
# CE3TSK 2026-09-20: builds packprobe against WSJT-X's OWN library.
#   sh build_packprobe.sh <WSJT-X 3.0.2 build directory> [output directory, default ./packprobe_build]
# The WSJT-X build directory is the one that holds libwsjt_fort.a, libwsjt_cxx.a and sftx after
# "cmake --build" of WSJT-X 3.0.2 from its sources. gfortran alone links it: packprobe needs the
# packer (libwsjt_fort.a) and the hash behind its CRC (nhash2, in libwsjt_cxx.a), nothing else.
# The output directory is NOT for a repository: the binary names the paths of the machine it was
# built on (the Fortran run-time's messages). .gitignore here keeps ./packprobe_build out.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
[ -n "$1" ] || { echo "usage: sh build_packprobe.sh <WSJT-X build directory> [output directory]"; exit 1; }
D=$(cd "$1" && pwd); O=${2:-$HERE/packprobe_build}; mkdir -p "$O"
for x in libwsjt_fort.a libwsjt_cxx.a sftx; do [ -e "$D/$x" ] || { echo "no $x in $D - build WSJT-X 3.0.2 there first"; exit 1; }; done
gfortran -O2 -fno-second-underscore -c "$HERE/packprobe.f90" -o "$O/packprobe.o"
gfortran "$O/packprobe.o" -o "$O/packprobe" "$D/libwsjt_fort.a" "$D/libwsjt_cxx.a"
echo "built $O/packprobe"
