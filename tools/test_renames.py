#!/usr/bin/env python3
"""
Tests for the rename tooling without IDA: runs ida_scripts/apply_renames.py
against a fake IDA API backed by the real unpacked executables.

    python3 tools/test_renames.py
"""
import io, os, runpy, sys, types, unittest, contextlib

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import renames_lib as R


class FakeIda:
    """Just enough of idc/ida_bytes/ida_funcs/ida_name for apply_renames.py."""

    def __init__(self, exe, idb):
        self.img = R.Image(exe)
        self.idb = idb
        self.names = {}
        self.funcs = set()
        self.func_cmts, self.cmts = {}, {}

    def install(self):
        s = self
        idc = types.ModuleType('idc')
        idc.SN_NOWARN, idc.SN_CHECK = 1, 2
        idc.get_idb_path = lambda: s.idb
        idc.add_func = lambda ea: (s.funcs.add(ea), True)[1]
        def set_name(ea, n, fl):
            if n in s.names.values() and s.names.get(ea) != n:
                return False
            s.names[ea] = n
            return True
        idc.set_name = set_name
        idc.get_func_cmt = lambda ea, rep: s.func_cmts.get(ea, '')
        idc.set_func_cmt = lambda ea, c, rep: s.func_cmts.__setitem__(ea, c)
        idc.get_cmt = lambda ea, rep: s.cmts.get(ea, '')
        idc.set_cmt = lambda ea, c, rep: s.cmts.__setitem__(ea, c)
        ib = types.ModuleType('ida_bytes')
        ib.is_loaded = lambda ea: 0 <= ea - R.LOAD_BASE < len(s.img.data)
        ib.get_bytes = lambda ea, n: s.img.bytes_at(ea, n)
        f = types.ModuleType('ida_funcs')
        f.get_func = lambda ea: types.SimpleNamespace(start_ea=ea) if ea in s.funcs else None
        n = types.ModuleType('ida_name')
        n.get_ea_name = lambda ea: s.names.get(ea, '')
        for m in (idc, ib, f, n):
            sys.modules[m.__name__] = m

    def run(self, **env):
        for k in ('CADAVER_RENAMES_DRY', 'CADAVER_RENAMES_MIN', 'CADAVER_RENAMES_FORCE'):
            os.environ.pop(k, None)
        os.environ.update(env)
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            runpy.run_path(os.path.join(ROOT, 'ida_scripts', 'apply_renames.py'), run_name='__main__')
        return out.getvalue()


def fake(game_exe='cadaver1/CAD.EXE', idb='disasm/cadaver1/cad.idb'):
    f = FakeIda(os.path.join(ROOT, 'unpacked', game_exe), os.path.join(ROOT, idb))
    f.install()
    return f


class Tests(unittest.TestCase):
    def test_apply_and_idempotent(self):
        f = fake()
        out = f.run()
        self.assertEqual(f.names[0x1C7D1], 'mjm_read')
        self.assertEqual(f.names[0x114E5], 'timer_isr')
        self.assertIn(0x114E5, f.funcs)                # unlabeled code got a function
        self.assertNotIn(0x1F727, f.names)             # 'guess' rows skipped by default
        self.assertIn('mjm_read', f.func_cmts[0x1C7D1] and 'mjm_read' or '')
        n = len(f.names)
        out2 = f.run()
        self.assertEqual(len(f.names), n)
        self.assertIn('renamed=0', out2)

    def test_min_conf_guess(self):
        f = fake(); f.run(CADAVER_RENAMES_MIN='guess')
        self.assertEqual(f.names[0x1F727], 'clock_div_count')

    def test_dry_run_changes_nothing(self):
        f = fake(); out = f.run(CADAVER_RENAMES_DRY='1')
        self.assertEqual(f.names, {}); self.assertIn('WOULD rename', out); self.assertIn('DRY RUN', out)

    def test_user_rename_is_not_overwritten(self):
        f = fake(); f.names[0x1C7D1] = 'my_own_name'
        out = f.run()
        self.assertEqual(f.names[0x1C7D1], 'my_own_name'); self.assertIn('CONFLICT', out)
        f.run(CADAVER_RENAMES_FORCE='1')
        self.assertEqual(f.names[0x1C7D1], 'mjm_read')

    def test_auto_and_old_names_are_replaced(self):
        f = fake(); f.names[0x1C7D1] = 'sub_1C7D1'; f.names[0x1C44A] = 'whatever'
        f.run()
        self.assertEqual(f.names[0x1C7D1], 'mjm_read')
        self.assertEqual(f.names[0x1C44A], 'whatever')   # not auto, not old -> conflict

    def test_wrong_build_is_refused(self):
        # Cadaver 1 table against the Cadaver 2 executable: signatures must stop it
        f = fake('cadaver2/GAME.EXE', 'disasm/cadaver1/cad.idb')
        out = f.run()
        self.assertIn('signature mismatch', out)
        self.assertLess(len(f.names), 15)

    def test_cadaver2_table_on_cadaver2_binary(self):
        f = fake('cadaver2/GAME.EXE', 'disasm/cadaver2/game.idb')
        out = f.run()
        self.assertEqual(f.names[0x1CEC6], 'mjm_read')
        self.assertEqual(f.names[0x1CB24], 'open_data_file')
        self.assertEqual(f.names[0x70F9F], 'path_routine_pak')
        self.assertNotIn('signature mismatch', out)
        self.assertNotIn('CONFLICT', out)

    def test_existing_comment_kept(self):
        f = fake(); f.funcs.add(0x1C7D1); f.func_cmts[0x1C7D1] = 'mine'
        f.run()
        self.assertEqual(f.func_cmts[0x1C7D1], 'mine')


class LibTests(unittest.TestCase):
    def test_sig(self):
        self.assertTrue(R.sig_matches(b'\x55\x8b\xec', '55 ?? EC'))
        self.assertFalse(R.sig_matches(b'\x55\x8b\xec', '55 8C'))

    def test_all_tables_parse_and_check(self):
        import subprocess
        r = subprocess.run([sys.executable, os.path.join(HERE, 'renames.py'), 'check'], capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)


if __name__ == '__main__':
    unittest.main(verbosity=2)
