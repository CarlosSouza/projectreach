import datetime
import hashlib
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'scripts'))
from device_profile import validate


class DeviceProfileTests(unittest.TestCase):
    def setUp(self):
        self.cert = b'test development certificate'
        self.sha = hashlib.sha1(self.cert).hexdigest().upper()
        self.profile = {
            'TeamIdentifier': ['VKDH2T9UTF'],
            'Entitlements': {
                'application-identifier': 'VKDH2T9UTF.dev.halopad.HaloPad',
                'com.apple.developer.team-identifier': 'VKDH2T9UTF',
                'get-task-allow': True,
                'com.apple.developer.kernel.extended-virtual-addressing': True,
                'com.apple.developer.kernel.increased-memory-limit': True,
            },
            'ExpirationDate': datetime.datetime(2027, 1, 1),
            'DeveloperCertificates': [self.cert],
            'ProvisionedDevices': ['DEVICE-1'],
        }
        self.now = datetime.datetime(2026, 9, 29)

    def check(self):
        return validate(self.profile, 'dev.halopad.HaloPad', self.sha, 'DEVICE-1', self.now)

    def test_matching_development_profile_passes(self):
        self.assertTrue(self.check()['com.apple.developer.kernel.extended-virtual-addressing'])

    def test_missing_memory_entitlement_fails(self):
        del self.profile['Entitlements']['com.apple.developer.kernel.extended-virtual-addressing']
        with self.assertRaisesRegex(ValueError, 'extended-virtual-addressing'):
            self.check()

    def test_wildcard_app_id_fails(self):
        self.profile['Entitlements']['application-identifier'] = 'VKDH2T9UTF.*'
        with self.assertRaisesRegex(ValueError, 'exactly'):
            self.check()

    def test_wrong_device_and_certificate_fail(self):
        self.profile['ProvisionedDevices'] = ['OTHER']
        with self.assertRaisesRegex(ValueError, 'device UDID'):
            self.check()
        self.profile['ProvisionedDevices'] = ['DEVICE-1']
        self.profile['DeveloperCertificates'] = [b'other']
        with self.assertRaisesRegex(ValueError, 'certificate'):
            self.check()

    def test_expired_or_distribution_profile_fails(self):
        self.profile['ExpirationDate'] = datetime.datetime(2026, 1, 1)
        with self.assertRaisesRegex(ValueError, 'expired'):
            self.check()
        self.profile['ExpirationDate'] = datetime.datetime(2027, 1, 1)
        self.profile['Entitlements']['get-task-allow'] = False
        with self.assertRaisesRegex(ValueError, 'development'):
            self.check()


if __name__ == '__main__':
    unittest.main()
