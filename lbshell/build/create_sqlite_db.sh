#!/bin/sh
# Given a sqlite database in text form, converts it to binary.
rm -fv "$2"
echo Converting $1 to $2
cat "$1" | sqlite3 "$2"
