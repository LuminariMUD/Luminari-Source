#!/bin/bash
cd $1
cp index index.backup
ls *.$1 |sort -n -k1 > index
echo $ >> index

