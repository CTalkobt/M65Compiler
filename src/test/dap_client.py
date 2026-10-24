#!/usr/bin/env python3
"""dap_client.py — a scripted Debug Adapter Protocol client for dbg45 --dap
(test_dbg45.sh): it plays an editor's part and checks the replies.

    dap_client.py dbg45 program.prg source.c xemu          a session (test_dbg45_xemu.c)
    dap_client.py dbg45 program.prg source.c xemu pause    pause a program that runs on
"""
import json
import subprocess
import sys

dbg45, prg, src, xemu = sys.argv[1:5]
mode = sys.argv[5] if len(sys.argv) > 5 else 'session'
p = subprocess.Popen([dbg45, '--dap'], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
seq = 0
bad = []


def send(cmd, args=None):
    global seq
    seq += 1
    body = json.dumps({'seq': seq, 'type': 'request', 'command': cmd, 'arguments': args or {}}).encode()
    p.stdin.write(b'Content-Length: %d\r\n\r\n' % len(body) + body)
    p.stdin.flush()
    return seq


def read():
    n = 0
    while True:
        line = p.stdout.readline()
        if not line:
            raise SystemExit('dbg45 --dap ended')
        line = line.strip()
        if not line:
            break
        if line.startswith(b'Content-Length:'):
            n = int(line[15:])
    return json.loads(p.stdout.read(n))


def until(pred, what):
    for _ in range(200):
        m = read()
        if pred(m):
            return m
    raise SystemExit('never got ' + what)


def request(cmd, args=None):
    s = send(cmd, args)
    return until(lambda m: m.get('type') == 'response' and m.get('request_seq') == s, cmd)


def event(name):
    return until(lambda m: m.get('type') == 'event' and m.get('event') == name, name)


def check(what, ok):
    if not ok:
        bad.append(what)


if mode == 'pause':
    # no breakpoints: it runs; a pause stops it where it is (main's loop)
    request('initialize', {'adapterID': 'cc45'})
    check('launch', request('launch', {'program': prg, 'headless': True, 'fast': True, 'xemu': xemu})['success'])
    request('configurationDone')
    import time
    time.sleep(3)
    request('pause', {'threadId': 1})
    e = event('stopped')
    check('a pause stops it', e['body']['reason'] == 'pause')
    st = request('stackTrace', {'threadId': 1})['body']['stackFrames']
    check('in main', st and st[0]['name'] == 'main')
    request('disconnect')
    p.wait(timeout=10)
    for b in bad:
        print('  ' + b)
    sys.exit(1 if bad else 0)

r = request('initialize', {'adapterID': 'cc45'})
check('initialize', r['success'] and r['body'].get('supportsConfigurationDoneRequest'))
r = request('launch', {'program': prg, 'headless': True, 'fast': True, 'xemu': xemu})
check('launch', r['success'])
r = request('setBreakpoints', {'source': {'path': src}, 'breakpoints': [{'line': 20}, {'line': 19}]})
bps = r['body']['breakpoints']
check('two breakpoints, line 19 moved to 20 (no code)', [b['verified'] for b in bps] == [True, True] and bps[1]['line'] == 20)
r = request('setFunctionBreakpoints', {'breakpoints': [{'name': 'add'}]})
check('function breakpoint', r['body']['breakpoints'][0]['verified'])
request('configurationDone')
e = event('stopped')
check('stops at a breakpoint', e['body']['reason'] == 'breakpoint')
st = request('stackTrace', {'threadId': 1})['body']['stackFrames']
check('in sum at line 20, called from main', [f['name'] for f in st] == ['sum', 'main'] and st[0]['line'] == 20 and
      st[0]['source']['path'].endswith('.c'))
sc = request('scopes', {'frameId': 0})['body']['scopes']
args = request('variables', {'variablesReference': sc[1]['variablesReference']})['body']['variables']
check('argument p', [a['name'] for a in args] == ['p'] and args[0]['type'] == '*struct:pt')
r = request('evaluate', {'expression': '*p', 'frameId': 0})
# (cc45 may call sum(&a) or sum(&b) first)
check('evaluate *p', r['success'] and r['body']['result'][:15] in ('{x = 3, y = -5,', '{x = 4, y = 6, '))
kids = request('variables', {'variablesReference': r['body']['variablesReference']})['body']['variables']
check('*p expands', [k['name'] for k in kids] == ['x', 'y', 'next'] and kids[1]['value'] in ('-5', '6'))
loc = request('variables', {'variablesReference': sc[0]['variablesReference']})['body']['variables']
check('locals of sum', 's' in [v['name'] for v in loc])
mainloc = request('variables', {'variablesReference': request('scopes', {'frameId': 1})['body']['scopes'][0]['variablesReference']})['body']['variables']
check('main\'s locals from its frame', [v['name'] for v in mainloc][:2] == ['a', 'b'])
request('stepOut', {'threadId': 1})
e = event('stopped')
st = request('stackTrace', {'threadId': 1})['body']['stackFrames']
check('step out to main', st[0]['name'] == 'main' and st[0]['line'] == 35)
request('setBreakpoints', {'source': {'path': src}, 'breakpoints': []})
request('setFunctionBreakpoints', {'breakpoints': []})
request('continue', {'threadId': 1})
e = event('exited')
check('exit status 11', e['body']['exitCode'] == 11)
event('terminated')
request('disconnect')
p.wait(timeout=10)
for b in bad:
    print('  ' + b)
sys.exit(1 if bad else 0)
