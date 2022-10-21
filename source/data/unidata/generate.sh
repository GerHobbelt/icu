#!/bin/bash

# © 2021 and later: Unicode, Inc. and others.
# License & terms of use: http://www.unicode.org/copyright.html

# This script runs Bazel to create (generate) header files and binary data files
# with Unicode character/script/collation data that are
# needed for bootstrapping the ICU4C build to integrate the data.

# Required environment variables:
#   - ICU_SRC - the root directory of ICU source. This directory contains the
#               `icu4c` directory.

ICU4C_COMMON=$ICU_SRC/icu4c/source/common
ICU4C_DATA_IN=$ICU_SRC/icu4c/source/data/in
ICU4C_NORM2=$ICU_SRC/icu4c/source/data/unidata/norm2

# Delete the files from the source tree that we need to generate,
# to make extra sure that we do not depend on their old versions for generating new ones.
# We cannot just delete *_data.h because ucol_data.h is not a generated header file.
rm $ICU_SRC/icu4c/source/common/norm2_nfc_data.h
rm $ICU_SRC/icu4c/source/common/propname_data.h
rm $ICU_SRC/icu4c/source/common/*_props_data.h
rm $ICU4C_DATA_IN/*.icu
rm $ICU4C_DATA_IN/*.nrm
rm $ICU4C_DATA_IN/coll/*.icu
# icu4c/source/i18n/collationfcd.cpp is generated by genuca;
# probably hard to build genuca without depending on the old version.

# Exit this shell script when a command fails.
set -e

# Generate normalization data files directly into the source tree.
bazelisk run //icu4c/source/tools/gennorm2 -- -o $ICU4C_COMMON/norm2_nfc_data.h -s $ICU4C_NORM2 nfc.txt --csource
bazelisk run //icu4c/source/tools/gennorm2 -- -o $ICU4C_DATA_IN/nfc.nrm         -s $ICU4C_NORM2 nfc.txt
bazelisk run //icu4c/source/tools/gennorm2 -- -o $ICU4C_DATA_IN/nfkc.nrm        -s $ICU4C_NORM2 nfc.txt nfkc.txt
bazelisk run //icu4c/source/tools/gennorm2 -- -o $ICU4C_DATA_IN/nfkc_cf.nrm     -s $ICU4C_NORM2 nfc.txt nfkc.txt nfkc_cf.txt
bazelisk run //icu4c/source/tools/gennorm2 -- -o $ICU4C_DATA_IN/uts46.nrm       -s $ICU4C_NORM2 nfc.txt uts46.txt

# genprops writes several files directly into the source tree.
bazelisk run //tools/unicode/c/genprops $ICU_SRC/icu4c

# genuca also writes several files directly into the source tree.
# We run it twice for different versions of the CLDR root sort order.
bazelisk run //tools/unicode/c/genuca -- --hanOrder implicit $ICU_SRC/icu4c
bazelisk run //tools/unicode/c/genuca -- --hanOrder radical-stroke $ICU_SRC/icu4c
# Also generate the ICU4X versions
bazelisk run //tools/unicode/c/genuca -- --icu4x --hanOrder implicit $ICU_SRC/icu4c
bazelisk run //tools/unicode/c/genuca -- --icu4x --hanOrder radical-stroke $ICU_SRC/icu4c
