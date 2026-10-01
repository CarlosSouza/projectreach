#!/usr/bin/env python3
"""Run upstream's stand-in machine with its current join-packet layout.

The 2026-10-01 engine added a 32-byte hardware id to the join packet;
upstream's test bot still sends the shorter packet. No gameplay simulation
is performed by this helper. Sources stay in the user's private checkout.
"""
import importlib.util
import pathlib

ENGINE = pathlib.Path(__file__).resolve().parents[2] / 'ref/xbox-build/vol/engine'
spec = importlib.util.spec_from_file_location('upstream_bots', ENGINE / 'tools/system_link_bots.py')
bots = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bots)

layout = (ENGINE / 'source/networking/network_messages.c').read_text()
if 'DEFINE_NETWORK_GAME_MESSAGE(message_client_join_game_request, 0x70)' in layout:
    def joined(self):
        hardware_id = f'{self.index:032x}'.encode('ascii')
        self.send(bots.message(bots.CLIENT_JOIN_GAME_REQUEST,
                               bots.wide(self.name, 32) + bots.JOIN_TOKEN + hardware_id))
        self.state = 'joining'
    bots.Machine.joined = joined

if __name__ == '__main__':
    bots.main()
