#!/bin/bash

# For use when building a debian package
steel_version_macro=$(grep 'define STEEL_VERSION' ../src/steel_version.h)
steel_version=$(echo "$steel_version_macro" | sed -e 's/.*"\(.*\)".*/\1/')
date=$( date +%Y-%m-%d )
date_id=$( date +%Y%m%d )

dch -v $steel_version-$date_id "$date daily build"
