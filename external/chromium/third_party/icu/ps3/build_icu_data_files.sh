#!/bin/bash

set -x
set -e

JOBS=16

# get a copy of icu4c, same version as in chromium (currently 4.6.1).
# This must be a source distribution, not a prepackaged tarball.

svn export http://source.icu-project.org/repos/icu/icu/tags/release-4-6/ icu

# configure it for static libs.
cd icu/source/

# populate our locale filters based our our particular project
# This updates the various res_index.res indexes. This has to be done because
# the indexes are generated before the filtering step (see the pkgdata line below).
# Therefore, without this, the indexes will say that we have a particular
# language, even when that language has been filtered out.
echo "BRK_RES_SOURCE = en.txt en_US.txt fi.txt ja.txt" > data/brkitr/brklocal.mk
echo "COLLATION_SOURCE = da.txt de.txt en.txt en_GB.txt en_US.txt es.txt fi.txt fr.txt it.txt ja.txt ko.txt nb.txt nl.txt pl.txt pt.txt pt_BR.txt pt_PT.txt ru.txt sv.txt zh.txt zh_Hans.txt zh_Hans_CN.txt zh_Hant.txt zh_Hant_TW.txt" > data/coll/collocal.mk
echo "GENRB_SOURCE = da.txt de.txt en.txt en_GB.txt en_US.txt es.txt fi.txt fr.txt it.txt ja.txt ko.txt nb.txt nl.txt pl.txt pt.txt pt_BR.txt pt_PT.txt ru.txt sv.txt zh.txt zh_Hans.txt zh_Hans_CN.txt zh_Hant.txt zh_Hant_TW.txt" > data/locales/reslocal.mk
echo "RBNF_SOURCE = da.txt de.txt en.txt es.txt fi.txt fr.txt it.txt ja.txt ko.txt nb.txt nl.txt pl.txt pt.txt pt_PT.txt ru.txt sv.txt zh.txt zh_Hant.txt" > data/rbnf/rbnflocal.mk

# run the configure
./configure --enable-static --disable-shared

# build the basic stuff:
mkdir -p bin lib
make -j ${JOBS} -C common/
make -j ${JOBS} -C i18n/
make -j ${JOBS} -C stubdata/
# build the packaging tool:
make -j ${JOBS} -C tools/toolutil/
make -j ${JOBS} -C tools/icupkg/
# build the data tools
make -j ${JOBS} -C tools/gencnval/
make -j ${JOBS} -C tools/genrb/
make -j ${JOBS} -C tools/gencfu/
make -j ${JOBS} -C tools/pkgdata/
make -j ${JOBS} -C tools/genbrk/
make -j ${JOBS} -C tools/genctd/
make -j ${JOBS} -C tools/makeconv/
make -j ${JOBS} -C tools/gensprep/

# build the ICU data files
make -j ${JOBS} -C data/

# variables (customize these):
repo_base=~/src/main
icutmpdir=/tmp/icutmpdir
icuintermediatedir=/tmp/icuintermediate
output="$repo_base/lbshell/content/icu"

# re-make our own .dat with only the pieces we want
# This is taken from one of the last lines of the make output from make -C data/
# If you want to change the contents of the data package, modify $output/icudata.lst before this next set of commands
cd data
mkdir -p $icuintermediatedir
mkdir -p $icutmpdir
PATH=../stubdata:../tools/ctestfw:../lib:$PATH  MAKEFLAGS= ../bin/pkgdata -O ../data/icupkg.inc -q -c -s ./out/build/icudt46l -d $icuintermediatedir -e icudt46  -T $icutmpdir -p icudt46l -m common -r 46.1 -L icudata $output/icudata.lst
rmdir $icutmpdir
cd ..

# clear old outputs:
rm -rf $output/icudt46b $output/icudt46l $output/icudata.lst $output/icudata_linux.lst
mkdir -p $output/icudt46b
mkdir -p $output/icudt46l

# run the packaging tool to update the outputs:
./bin/icupkg -tb $icuintermediatedir/icudt46l.dat -d $output/icudt46b --list -x \* /tmp/icudt46b.dat -o $output/icudata.lst
./bin/icupkg $icuintermediatedir/icudt46l.dat -d $output/icudt46l --list -x \* /tmp/icudt46l.dat -o $output/icudata_linux.lst

# back to the original directory
cd ../../

# You can keep $icuintermediatedir around in case you want the little-endian .dat file
# If you don't want it...
rm -rf $icuintermediatedir

# clean up the source checkout (if you want)
rm -rf icu

# you're done!  just review changes and submit.
