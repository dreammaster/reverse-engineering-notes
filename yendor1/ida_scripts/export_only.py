"""
No-op target script for batch_run_and_export.py: does nothing itself, so the
driver's analysis-wait + .asm/.idc export + save is the whole job. Used by
create_idb.ps1 for the initial database build, and to re-export after a
GUI session.
"""

print("export_only: nothing to do, the driver exports after this returns")
