#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

make clean
make
sudo ./bin/krz-dns-filter /home/lab/krz-dns-filter/blacklist.txt 53
