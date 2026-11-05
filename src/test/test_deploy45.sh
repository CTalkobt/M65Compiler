#!/bin/bash
# deploy45 (#145): the commands it runs (stand-in etherload, m65 and
# mega65_ftp that record their arguments) and the Ultimate 64 requests it
# sends (a local HTTP server standing in for the U64's API)

D45=./bin/deploy45
T=build/test/deploy45
rm -rf $T; mkdir -p $T
passed=0
failed=0
pass() { echo "PASS: $1"; passed=$((passed + 1)); }
fail() { echo "FAIL: $1"; failed=$((failed + 1)); }
unset M65_HOST M65_DEVICE M65_SPEED U64_HOST U64_PASSWORD ETHERLOAD M65 MEGA65_FTP

# the stand-ins: their arguments into $T/args
for t in etherload m65 mega65_ftp; do
    printf '#!/bin/sh\necho "%s $*" > %s/args\n' "$t" "$PWD/$T" > $T/$t
    chmod +x $T/$t
done
printf '#!/bin/sh\nsleep 30\n' > $T/slow; chmod +x $T/slow
printf '\x01\x08\x0b\x08\x00\x00' > $T/c64.prg          # loads at $0801
printf '\x01\x20\x0b\x20\x00\x00' > $T/m65.prg          # loads at $2001 (cc45's)
head -c 819200 /dev/zero > $T/my_game.d81
head -c 174848 /dev/zero > $T/disk.d64

# expect "<description>" "<what the stand-in got>" deploy45 args...
expect() {
    local what="$1" want="$2"; shift 2
    rm -f $T/args
    if "$D45" "$@" > $T/out 2>&1 && [ "$(cat $T/args 2>/dev/null)" = "$want" ]; then pass "$what"
    else fail "$what: got '$(cat $T/args 2>/dev/null)' ($(head -c 200 $T/out)), wanted '$want'"; fi
}
expect "m65ether: a MEGA65 program, reset to MEGA65 mode, run" "etherload -5 -r $T/m65.prg" m65ether --tool $T/etherload $T/m65.prg
expect "m65ether: a \$0801 program in C64 mode, -i" "etherload -i 192.168.1.65 -4 -r $T/c64.prg" m65ether --tool $T/etherload -i 192.168.1.65 $T/c64.prg
expect "m65ether: --no-run, --c64 (forced)" "etherload -4 $T/m65.prg" m65ether --tool $T/etherload --no-run --c64 $T/m65.prg
M65_HOST=fe80::1%eth0 expect "m65ether: \$M65_HOST" "etherload -i fe80::1%eth0 -5 -r $T/m65.prg" m65ether --tool $T/etherload $T/m65.prg
ETHERLOAD=$T/etherload expect "m65ether: \$ETHERLOAD" "etherload -5 -r $T/m65.prg" m65ether $T/m65.prg
expect "m65serial: -l, -s, reset, run" "m65 -l /dev/ttyUSB1 -s 2000000 -F -r $T/m65.prg" m65serial --tool $T/m65 -l /dev/ttyUSB1 -s 2000000 $T/m65.prg
expect "m65serial: C64 mode, --no-reset" "m65 -4 -r $T/c64.prg" m65serial --tool $T/m65 --no-reset $T/c64.prg
M65_DEVICE=/dev/ttyUSB2 expect "m65serial: \$M65_DEVICE" "m65 -l /dev/ttyUSB2 -F -r $T/m65.prg" m65serial --tool $T/m65 $T/m65.prg
expect "m65mount: put and mount over serial, the card's 8.3 name" "mega65_ftp -l /dev/ttyUSB1 -c put $T/my_game.d81 MY_GAME.D81 -c mount MY_GAME.D81 -c exit" m65mount --tool $T/mega65_ftp -l /dev/ttyUSB1 $T/my_game.d81
expect "m65mount: over Ethernet (-i), --name" "mega65_ftp -e -i 10.0.0.65 -c put $T/my_game.d81 GAME.D81 -c mount GAME.D81 -c exit" m65mount --tool $T/mega65_ftp -i 10.0.0.65 --name GAME.D81 $T/my_game.d81
expect "m65mount: --ether, discovered" "mega65_ftp -e -c put $T/my_game.d81 MY_GAME.D81 -c mount MY_GAME.D81 -c exit" m65mount --tool $T/mega65_ftp --ether $T/my_game.d81
expect "m65mount: --on-card" "mega65_ftp -l /dev/ttyUSB1 -c mount MEGA65.D81 -c exit" m65mount --tool $T/mega65_ftp -l /dev/ttyUSB1 --on-card MEGA65.D81
expect "m65mount: --virtual (m65 -d)" "m65 -l /dev/ttyUSB1 -d $T/my_game.d81" m65mount --tool $T/m65 -l /dev/ttyUSB1 --virtual $T/my_game.d81

# refused, with a message
refuse() {
    local what="$1" msg="$2"; shift 2
    if "$D45" "$@" > $T/out 2>&1; then fail "$what: accepted"
    elif grep -q "$msg" $T/out; then pass "$what"
    else fail "$what: not '$msg': $(head -c 200 $T/out)"; fi
}
refuse "m65mount: a .d64" "mounts .d81" m65mount --tool $T/mega65_ftp $T/disk.d64
cp $T/my_game.d81 "$T/my game.d81"
refuse "m65mount: a name with a space" "no spaces" m65mount --tool $T/mega65_ftp -l /dev/x "$T/my game.d81"
refuse "m65mount --virtual over Ethernet" "serial monitor" m65mount --tool $T/m65 -i 1.2.3.4 --virtual $T/my_game.d81
refuse "m65ether: a missing file" "cannot read" m65ether --tool $T/etherload $T/none.prg
refuse "an unknown command" "unknown command" m65usb $T/m65.prg
refuse "u64: no host" "no host" u64 $T/m65.prg
refuse "m65ether: a tool that never finishes, --timeout" "did not finish in 1 s" m65ether --tool $T/slow --timeout 1 $T/m65.prg
refuse "m65serial: a tool not there" "cannot run" m65serial --tool $T/no-such-m65 $T/m65.prg
if "$D45" m65ether --tool $T/slow --timeout 1 $T/m65.prg > /dev/null 2>&1; [ $? -eq 124 ]; then pass "timeout: exit status 124"; else fail "timeout: exit status"; fi

# dry runs
out=$("$D45" m65ether -n -i 1.2.3.4 --tool etherload $T/m65.prg)
[ "$out" = "etherload -i 1.2.3.4 -5 -r $T/m65.prg" ] && pass "--dry-run prints the command" || fail "--dry-run: $out"
out=$("$D45" u64 -n -i u64.local --password pw $T/m65.prg)
[ "$out" = "POST http://u64.local/v1/runners:run_prg (6 bytes) X-Password" ] && pass "u64 --dry-run prints the request" || fail "u64 --dry-run: $out"

# the Ultimate 64: a local server records method, path, X-Password, the body's size and first bytes
cat > $T/u64.py <<'PY'
import http.server, sys, json
log = open(sys.argv[1], 'a')
class H(http.server.BaseHTTPRequestHandler):
    def handle_any(self):
        n = int(self.headers.get('Content-Length') or 0)
        body = self.rfile.read(n) if n else b''
        log.write(f"{self.command} {self.path} pw={self.headers.get('X-Password')} len={len(body)} head={body[:2].hex()}\n"); log.flush()
        if self.headers.get('X-Password') == 'wrong':
            self.send_response(403); self.end_headers(); return
        if self.path == '/v1/info':
            b = json.dumps({"product": "Ultimate 64", "errors": []}).encode()
            self.send_response(200); self.send_header('Content-Length', str(len(b))); self.end_headers(); self.wfile.write(b); return
        self.send_response(200); self.send_header('Content-Length', '0'); self.end_headers()
    do_GET = do_POST = do_PUT = handle_any
    def log_message(self, *a): pass
s = http.server.HTTPServer(('127.0.0.1', 0), H)
open(sys.argv[2], 'w').write(str(s.server_address[1]))
s.serve_forever()
PY
python3 $T/u64.py $T/u64.log $T/u64.port &
SRV=$!
for i in $(seq 50); do [ -s $T/u64.port ] && break; sleep 0.1; done
H=127.0.0.1:$(cat $T/u64.port)
u64() {
    local what="$1" want="$2"; shift 2
    : > $T/u64.log
    if "$D45" u64 -i $H "$@" > $T/out 2>&1 && [ "$(cat $T/u64.log)" = "$want" ]; then pass "$what"
    else fail "$what: got '$(cat $T/u64.log)' ($(head -c 200 $T/out)), wanted '$want'"; fi
}
u64 "u64: a PRG run (POST runners:run_prg, the password)" "POST /v1/runners:run_prg pw=secret len=6 head=0120" --password secret $T/m65.prg
U64_PASSWORD=env u64 "u64: --no-run loads (runners:load_prg), \$U64_PASSWORD" "POST /v1/runners:load_prg pw=env len=6 head=0108" --no-run $T/c64.prg
u64 "u64: a d64 mounted on drive b" "POST /v1/drives/b:mount?type=d64&mode=readwrite pw=None len=174848 head=0000" --drive b $T/disk.d64
u64 "u64: --reset (PUT machine:reset)" "PUT /v1/machine:reset pw=None len=0 head=" --reset
u64 "u64: --info (GET v1/info)" "GET /v1/info pw=None len=0 head=" --info
grep -q '"product": "Ultimate 64"' $T/out && pass "u64 --info prints the answer" || fail "u64 --info output: $(cat $T/out)"
if "$D45" u64 -i $H --password wrong $T/m65.prg > $T/out 2>&1; then fail "u64: a wrong password accepted"
elif grep -q "wrong network password" $T/out; then pass "u64: a wrong password (403)"; else fail "u64 403: $(cat $T/out)"; fi
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
if "$D45" u64 -i $H --timeout 2 $T/m65.prg > $T/out 2>&1; then fail "u64: nothing listening, accepted"
elif grep -q "cannot connect" $T/out; then pass "u64: nothing listening"; else fail "u64 no server: $(cat $T/out)"; fi
refuse "u64: a file it cannot send" "not a PRG" u64 -i $H $T/u64.py

echo ""
echo "deploy45 tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
