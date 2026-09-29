"""The one paired iOS device in `xcrun devicectl list devices --json-output` output, or nothing."""
import json, sys
devices = [d for d in json.load(open(sys.argv[1]))['result']['devices']
           if d.get('hardwareProperties', {}).get('platform') == 'iOS'
           and d.get('connectionProperties', {}).get('pairingState') == 'paired']
print(devices[0]['identifier'] if len(devices) == 1 else '')
