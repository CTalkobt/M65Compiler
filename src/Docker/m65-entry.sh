#!/bin/sh
# The entrypoint of the cc45 Docker images with xemu (#99): when a MEGA65 ROM
# is mounted, set up xemu's preferences directory before the command runs
# (so test scripts that copy it for each run find it ready), then run the
# command.
ROM=${MEGA65_ROM:-/rom/MEGA65.ROM}
: "${XEMU_PREF_DIR:=/tmp/xemu-pref}"
export XEMU_PREF_DIR
if [ -r "$ROM" ] && [ ! -e "$XEMU_PREF_DIR/mega65.img" ]; then
    # (the wrapper does it; -h only prints the options)
    xemu-xmega65 -h >/dev/null 2>&1
fi
exec "$@"
