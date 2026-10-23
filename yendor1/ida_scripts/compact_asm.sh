#!/bin/sh
# usage: compact_asm.sh progN  -> /tmp/progN.compact.txt (comments, blank lines and XREF noise removed) for reading
n=$1
cd "$(dirname "$0")/.." || exit 1
grep -v "^ *; \(\|[A-Z]\)\|^$\|^; ---\|^ *;$" $n.asm | sed -E 's/ {2,}; (DATA|CODE) XREF.*$//' | grep -v "^ *; \|^ *;$" > /tmp/$n.compact.txt
wc -l /tmp/$n.compact.txt
