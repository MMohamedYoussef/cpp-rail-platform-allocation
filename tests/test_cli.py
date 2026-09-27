import pathlib
import subprocess
import sys
import tempfile
import unittest

BIN = pathlib.Path(sys.argv.pop(1)).resolve()


class PlatformTests(unittest.TestCase):
    def run_cli(self, data):
        with tempfile.TemporaryDirectory() as directory:
            source = pathlib.Path(directory) / "trains.csv"
            target = pathlib.Path(directory) / "allocation.csv"
            source.write_text(data, encoding="utf-8")
            result = subprocess.run([str(BIN), str(source), str(target)], capture_output=True, text=True)
            output = target.read_text(encoding="utf-8") if target.exists() else None
            return result, output

    def test_assigns_minimum_platforms_and_reuses_at_departure(self):
        result, output = self.run_cli("train,arrival,departure\nA,09:00,09:30\nB,09:10,09:40\nC,09:30,10:00\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output, "train,platform,arrival,departure\nA,1,09:00,09:30\nB,2,09:10,09:40\nC,1,09:30,10:00\n")
        self.assertEqual(result.stdout.strip(), "platforms=2 trains=3")

    def test_rejects_zero_duration(self):
        result, output = self.run_cli("train,arrival,departure\nA,09:00,09:00\n")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("departure", result.stderr.lower())
        self.assertIsNone(output)

    def test_rejects_duplicate_train(self):
        result, _ = self.run_cli("train,arrival,departure\nA,09:00,09:30\nA,09:30,10:00\n")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("duplicate", result.stderr.lower())

    def test_rejects_formula_like_train_ids(self):
        for train in ("=1+1", "+2", "-2", "@SUM", "  =1+1"):
            with self.subTest(train=train):
                result, output = self.run_cli(f"train,arrival,departure\n{train},09:00,09:30\n")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("formula", result.stderr.lower())
                self.assertIsNone(output)

    def test_rejects_existing_output_without_temporary_residue(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "trains.csv"
            target = root / "allocation.csv"
            source.write_text("train,arrival,departure\nA,09:00,09:30\n", encoding="utf-8")
            target.write_text("keep", encoding="utf-8")
            existing = subprocess.run([str(BIN), str(source), str(target)], capture_output=True, text=True)
            self.assertNotEqual(existing.returncode, 0)
            self.assertEqual(target.read_text(encoding="utf-8"), "keep")
            self.assertEqual(list(root.glob("allocation.csv.tmp-*")), [])

    def test_refuses_dangling_symlink_output(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "trains.csv"
            target = root / "allocation.csv"
            missing = root / "missing.csv"
            source.write_text("train,arrival,departure\nA,09:00,09:30\n", encoding="utf-8")
            try:
                target.symlink_to(missing)
            except (OSError, NotImplementedError):
                self.skipTest("creating symlinks is unavailable on this host")
            result = subprocess.run([str(BIN), str(source), str(target)], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertTrue(target.is_symlink())
            self.assertFalse(missing.exists())
            self.assertEqual(list(root.glob("allocation.csv.tmp-*")), [])


if __name__ == "__main__":
    unittest.main()
