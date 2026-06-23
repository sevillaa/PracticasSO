#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
	echo "Usage: $0 <directorio>" >&2
	exit 1
fi

dir=$1

if [[ -e "$dir" && ! -d "$dir" ]]; then
	echo "Error: $dir existe pero no es un directorio" >&2
	exit 1
fi

mkdir -p -- "$dir"

shopt -s dotglob nullglob
rm -rf -- "$dir"/*
shopt -u dotglob nullglob

cd -- "$dir"

mkdir subdir
touch fichero1
printf "1234567890" > fichero2
ln -s fichero2 enlaceS
ln fichero2 enlaceH

for file in subdir fichero1 fichero2 enlaceS enlaceH; do
	echo "===== $file ====="
	stat -- "$file"
	echo
done

echo "===== subdir/. ====="
stat -- subdir/.
echo

echo "===== subdir/.. ====="
stat -- subdir/..
