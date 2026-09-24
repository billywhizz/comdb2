#!/bin/bash
set -euo pipefail

export PATH=$PATH:$(pwd)/build/db

IMG=4096.img
MNT=/tmp/mnt4096
DBNAME=reprodb
WAIT=${WAIT:-6}
SECTORS=${SECTORS:-4096}
loop=
log=

cleanup() {
  rc=$?
  set +e
  if mountpoint -q "$MNT"; then
    sudo umount "$MNT" || sudo umount -l "$MNT"
  fi
  mountpoint -q "$MNT" || rm -rf "$MNT"
  [ -n "$loop" ] && sudo losetup -d "$loop"
  rm -f "$IMG" "$log"
  echo cleaned up
  exit "$rc"
}

trap cleanup EXIT INT TERM
log=$(mktemp)
rm -f "$IMG"
truncate -s 512M "$IMG"
loop=$(sudo losetup -f --show -b "$SECTORS" "$IMG")
sudo mkfs.xfs -q -f "$loop"
mkdir -p "$MNT"
sudo mount "$loop" "$MNT"
sudo chown -R andrew:andrew "$MNT"
comdb2 --create "$DBNAME" --dir "$MNT/db"
#comdb2 --create "$DBNAME" --dir "$MNT/db" --tunable "setattr directio 0"
echo database created
rc=0
timeout "$WAIT" comdb2 "$DBNAME" --dir "$MNT/db" --tunable "port localhost 21000" --tunable "portmux_port 0" --tunable "disallow_portmux_route 1" 2>&1 | tee "$log" || rc=$?
#timeout "$WAIT" comdb2 "$DBNAME" --dir "$MNT/db" --tunable "port localhost 21000" --tunable "portmux_port 0" --tunable "disallow_portmux_route 1" --tunable "setattr directio 0" 2>&1 | tee "$log" || rc=$?
if grep -q "Failed to open low level meta table" "$log"; then
  echo "REPRODUCED: comdb2 exited $rc on 4096-byte logical sectors"
  exit 0
fi
if [ "$rc" -eq 124 ]; then
  echo "NOT REPRODUCED: comdb2 stayed up for ${WAIT}s on 4kn storage" >&2
else
  echo "UNEXPECTED: comdb2 exited $rc without the expected fatal error" >&2
fi
exit 1
