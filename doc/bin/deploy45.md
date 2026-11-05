# deploy45 — Programs and disk images to a MEGA65 or an Ultimate 64

`deploy45` sends a program (or a disk image) to real hardware: a MEGA65 over its Ethernet port or through the
serial port of its JTAG adapter, or an Ultimate 64 over the network. For the MEGA65 it runs the
[mega65-tools](https://github.com/mega65/mega65-tools) programs (`etherload`, `m65`, `mega65_ftp`), so their
protocols stay theirs; for the Ultimate 64 it speaks the device's HTTP API itself.

## Usage

```bash
deploy45 m65ether  [options] <prog.prg>     # Ethernet: etherload
deploy45 m65serial [options] <prog.prg>     # the JTAG adapter's serial port: m65
deploy45 m65mount  [options] <image.d81>    # an image onto the SD card, mounted: mega65_ftp
deploy45 m65mount  --virtual -l DEV <image.d81>   # the image served from this computer: m65 -d
deploy45 u64       [options] <file>         # an Ultimate 64: a PRG, CRT, SID or disk image
deploy45 -V | --version
deploy45 -h | --help
```

| Option | Description |
|--------|-------------|
| `-i`, `--host HOST` | The MEGA65's address (`etherload -i` / `mega65_ftp -i`: an IPv4 or IPv6 address, `fe80::…%eth0`; default `$M65_HOST`, else the tool discovers the MEGA65), or the Ultimate 64's (`name[:port]`, `$U64_HOST`) |
| `-l`, `--device DEV` | The serial port (`/dev/ttyUSB1`; default `$M65_DEVICE`, else the one `m65` finds) |
| `-s`, `--speed BPS` | The serial speed (`$M65_SPEED`; else the tool's default, 2000000) |
| `--c64`, `--m65` | The mode the program starts in. Default: C64 mode for a program loading at `$0801`, MEGA65 mode otherwise (cc45's programs load at `$2001`) |
| `--no-run` | Load without running |
| `--no-reset` | No reset before loading (`m65ether`, `m65serial`) |
| `--name NAME.D81` | `m65mount`: the name on the SD card. Default: the file's name in 8.3 upper case (`my_game.d81` → `MY_GAME.D81`) |
| `--on-card NAME.D81` | `m65mount`: mount an image that is on the SD card already (nothing uploaded) |
| `--virtual` | `m65mount`: `m65 -d` — the MEGA65's drive reads and writes the image on this computer through the serial monitor, while `deploy45` runs (Ctrl-C ends it) |
| `--ether` | `m65mount`: over Ethernet (`mega65_ftp -e`) when no `-i` is given: the MEGA65 is discovered |
| `--password PW` | `u64`: the network password (`$U64_PASSWORD`), sent as `X-Password` |
| `--drive a\|b` | `u64`: the drive a disk image is mounted on (default `a`) |
| `--song N` | `u64`: the song of a SID to play |
| `--info` | `u64`: print the device's information (`GET v1/info`) |
| `--reset` | `u64`: reset the machine (`PUT v1/machine:reset`), with no file |
| `--tool PATH` | The mega65-tools program to run. Default: `$ETHERLOAD`, `$M65` or `$MEGA65_FTP`, else the one on the `PATH`, else `~/mega65-tools/bin/` or `~/mega65-tools/` |
| `--timeout SEC` | Give up after SEC seconds (exit status 124); 0: never. Default 60 for loading (`etherload` waits forever for a MEGA65 that is off), 30 for `u64`, none for `m65mount` (an upload over serial runs at about 5 KB/s) |
| `-n`, `--dry-run` | Print the command (or the HTTP request) instead of running it |
| `-v` … `-vvvv` | Log on stderr as `[deploy45:LEVEL] …` |

## MEGA65 over Ethernet: `m65ether`

```bash
deploy45 m65ether game.prg                   # reset to MEGA65 mode, load, RUN
deploy45 m65ether -i 192.168.1.65 tool.prg
deploy45 m65ether --c64 --no-run c64game.prg
```

Runs `etherload [-i HOST] -5|-4 [-r] prog.prg` (`-4`: C64 mode, `-5`: MEGA65 mode). The MEGA65 must be waiting
for it: its Ethernet loader is started from the MEGA65 (see the MEGA65 Book's chapter on data transfer and the
mega65-tools documentation).

## MEGA65 through the JTAG adapter: `m65serial`

```bash
deploy45 m65serial -l /dev/ttyUSB1 game.prg
M65_DEVICE=/dev/ttyUSB1 deploy45 m65serial --c64 c64game.prg
```

Runs `m65 [-l DEV] [-s BPS] [-F] [-4] [-r] prog.prg`. The "serial" connection is the serial port of the JTAG
adapter (TE-0790) on the MEGA65's JB1 header — the serial monitor the m65 tool works through. `-F` (a reset) is left
out with `--no-reset`; `-4` starts C64 mode. Like `m65`, it loads through BASIC's `LOAD`, so the program must fit
the lower 64 KB.

## MEGA65 disk images: `m65mount`

```bash
deploy45 m65mount -l /dev/ttyUSB1 game.d81           # put GAME.D81 on the card, mount it
deploy45 m65mount -i 192.168.1.65 game.d81           # the same over Ethernet
deploy45 m65mount -l /dev/ttyUSB1 --on-card MEGA65.D81
deploy45 m65mount -l /dev/ttyUSB1 --virtual work.d81 # no copy: the drive is the local file
```

Runs `mega65_ftp {-l DEV [-s BPS] | -e [-i HOST]} -c "put image NAME.D81" -c "mount NAME.D81" -c exit`: the image
is written to the SD card's root and mounted as drive 8. The MEGA65 mounts only `.d81` (and `.d65`) images, and only
unfragmented ones. mega65_ftp should be used at the `READY.` prompt (it takes over the machine while it works), and
its `put` needs a file name without spaces. `--virtual` runs `m65 -l DEV -d image.d81` instead: nothing is copied;
the MEGA65's drive reads and writes the local file through `m65` for as long as it runs.

## Ultimate 64: `u64`

```bash
deploy45 u64 -i u64.local game.prg                    # reset, DMA load, RUN
deploy45 u64 -i 192.168.1.64 --password secret --no-run tool.prg
deploy45 u64 -i u64.local cart.crt                    # the cartridge started
deploy45 u64 -i u64.local --song 2 tune.sid
deploy45 u64 -i u64.local --drive b disk.d64          # mounted on drive b
deploy45 u64 -i u64.local --info
deploy45 u64 -i u64.local --reset
```

The Ultimate 64's REST API over HTTP: `POST v1/runners:run_prg` (or `load_prg` with `--no-run`), `v1/runners:run_crt`,
`v1/runners:sidplay`, `v1/drives/<a|b>:mount?type=<d64|d71|d81|g64|g71>&mode=readwrite` with the file as the body;
`GET v1/info`; `PUT v1/machine:reset`. A device with a network password refuses a request without it (HTTP 403:
"wrong network password").

## Exit Status

0 when the transfer succeeded; the mega65-tools program's own status when it failed; 124 when it did not finish
within `--timeout`; 127 when it could not be started (not installed: name it with `--tool`); 1 for `deploy45`'s own
errors (an unreadable file, an HTTP error, no host).

## See Also

- The MEGA65 Book, "Data Transfer and Debugging Tools" (m65, mega65_ftp, M65Connect)
- [mega65-tools](https://github.com/mega65/mega65-tools) — etherload, m65, mega65_ftp
- [dbg45](dbg45.md) — debugging a program in xemu
- `src/test/test_deploy45.sh` — the commands run and requests sent, against stand-ins
