"""Lists public Halo Custom Edition 1.10 servers from the master server's list on stdin
(halo-query -a output), most populated first, skipping full and password-protected ones.
Usage: node cli.js ce -a | python3 scripts/public-servers.py CLI_JS N"""
import json
import subprocess
import sys

addrs = [f"{s['address']}:{s['port']}" for s in json.load(sys.stdin)]
rows = []
for i in range(0, len(addrs), 40):
    out = subprocess.run(['node', sys.argv[1], *addrs[i:i + 40], '-r', '-t', '3000'], capture_output=True, text=True).stdout
    for line in out.splitlines():
        f = line.split('\\')[1:]
        d = dict(zip(f[0::2], f[1::2]))
        if d.get('gamever') == '01.00.10.0621' and d.get('password') == '0' and int(d.get('numplayers', 0)) < int(d.get('maxplayers', 16)):
            rows.append((int(d.get('numplayers', 0)), d['ip'] + ':' + d['port']))
for n, a in sorted(rows, reverse=True)[:int(sys.argv[2])]:
    print(a)
