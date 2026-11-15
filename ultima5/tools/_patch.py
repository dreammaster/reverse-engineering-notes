import sys
def patch(path, old, new, count=1):
    b = open(path, 'rb').read().decode('latin-1')
    crlf = '\r\n' in b
    o = old.replace('\n', '\r\n') if crlf else old
    n = new.replace('\n', '\r\n') if crlf else new
    c = b.count(o)
    if c != count:
        sys.exit("%s: expected %d match(es), found %d for: %s" % (path, count, c, old[:60]))
    open(path, 'wb').write(b.replace(o, n).encode('latin-1'))
