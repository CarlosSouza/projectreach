"""Only the two installer-written values may go into a personal app (build-ios-app.py --product-id)."""
import importlib.util
import pathlib
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
spec = importlib.util.spec_from_file_location('ios_builder_pid', ROOT / 'scripts/build-ios-app.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)
KEY = 'HKLM\\Software\\Microsoft\\Microsoft Games\\Halo CE'
PID = f'value {KEY} 1 504944 3030303030ff00'                       # fixture bytes, not a real ID
DPID = f'value {KEY} 3 4469676974616c50726f647563744944 a4000000'


class ProductIdTests(unittest.TestCase):
    def check(self, text):
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / 'product-id.txt'
            path.write_text(text)
            return builder.checked_product_id(path)

    def test_both_values_accepted_comments_dropped(self):
        self.assertEqual(self.check(f'# private\n{PID}\n{DPID}\n'), f'{PID}\n{DPID}\n')

    def test_missing_value_rejected(self):
        with self.assertRaisesRegex(ValueError, 'both PID and DigitalProductID'):
            self.check(PID + '\n')

    def test_other_values_rejected(self):
        for line in (f'value {KEY} 4 4649525354 01000000',                       # FIRSTRUN
                     f'value HKCU\\Software\\Microsoft\\Microsoft Games\\Halo CE 1 504944 00',
                     'key HKLM\\Software'):
            with self.assertRaisesRegex(ValueError, 'not a product ID'):
                self.check(f'{PID}\n{DPID}\n{line}\n')


if __name__ == '__main__':
    unittest.main()
