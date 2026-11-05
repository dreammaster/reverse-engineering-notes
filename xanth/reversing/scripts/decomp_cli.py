import sys
sys.argv, _argv = [sys.argv[0]], sys.argv
import decomp_gen
from decomp_emit import function_text
sys.argv = _argv
ctx = decomp_gen.Ctx()
for n in sys.argv[1:]:
    print(function_text(ctx, n))
    print()
