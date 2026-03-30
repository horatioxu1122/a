#!/bin/sh
# Copy all winls scripts to OneDrive Desktop
d="/mnt/c/Users/these/OneDrive/Desktop"
for f in "$(dirname "$0")"/*.bat "$(dirname "$0")"/*.ps1; do
  [ -f "$f" ] && cp "$f" "$d/" && echo "$(basename "$f") -> Desktop"
done
