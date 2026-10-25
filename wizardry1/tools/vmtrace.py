"""trace proc entries of the p-code run: vmtrace.py [steps] [keys]"""
import sys, time
sys.path.insert(0, __import__('os').path.dirname(__file__))
import wizvm
steps = int(sys.argv[1]) if len(sys.argv) > 1 else 2_000_000
keys = sys.argv[2].encode('latin1').decode('unicode_escape').encode('latin1') if len(sys.argv) > 2 else b''
vm = wizvm.VM(keys=keys)
vm.start()
orig = vm.enter
cnt = {}
order = []
def enter(key, mode):
    k = (vm.segs[key[0]].name, key[1])
    if k not in cnt:
        order.append(k); cnt[k] = 0
        print('first call', k, 'depth-from', vm.frame.proc.num, 'ip', hex(vm.ip))
    cnt[k] += 1
    return orig(key, mode)
vm.enter = enter
t = time.time()
try:
    vm.run(steps)
except Exception as e:
    print('stop', repr(e), vm.seg.name, vm.frame.proc.num, hex(vm.ip))
print('elapsed', time.time() - t, 'steps', vm.steps)
top = sorted(cnt.items(), key=lambda kv: -kv[1])[:10]
print(top)
