#!/bin/sh
# xemu-xmega65 in the cc45 Docker images (#99): the MEGA65 emulator, its
# preferences directory set up from your MEGA65 ROM on first use.
#
# The ROM cannot be shipped: mount yours at /rom/MEGA65.ROM
#   docker run -v /path/to/MEGA65.ROM:/rom/MEGA65.ROM:ro ...
# or name it with MEGA65_ROM. The preferences directory ($XEMU_PREF_DIR,
# default /tmp/xemu-pref) gets an SD card image made by disk45 as the
# MEGA65's FDISK lays one out, configured as after the first-boot onboarding
# (else the hypervisor waits in it), with the ROM on it.
#
# xemu itself is at /usr/local/libexec/xmega65 (built from CTalkobt/xemu
# cc45-dev: XEMU_PREF_DIR, the monitor's breakpoint list for dbg45, the
# hardware divider as the core computes it).

REAL=/usr/local/libexec/xmega65
ROM=${MEGA65_ROM:-/rom/MEGA65.ROM}
: "${XEMU_PREF_DIR:=/tmp/xemu-pref}"
export XEMU_PREF_DIR

if [ ! -e "$XEMU_PREF_DIR/mega65.img" ]; then
    if [ ! -r "$ROM" ]; then
        echo "xemu-xmega65: no MEGA65 ROM at $ROM: mount yours (docker run -v /path/to/MEGA65.ROM:$ROM:ro ...) or set MEGA65_ROM" >&2
        exit 2
    fi
    mkdir -p "$XEMU_PREF_DIR" &&
    disk45 sd create "$XEMU_PREF_DIR/mega65.img" 4G --onboarded --pal >/dev/null &&
    disk45 sd add "$XEMU_PREF_DIR/mega65.img" "$ROM" MEGA65.ROM >/dev/null &&
    cp "$ROM" "$XEMU_PREF_DIR/MEGA65.ROM" || {
        echo "xemu-xmega65: cannot set up $XEMU_PREF_DIR" >&2
        exit 2
    }
fi
exec "$REAL" "$@"
