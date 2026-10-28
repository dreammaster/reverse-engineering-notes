#!/usr/bin/env python3
"""Turn a Claude Code session transcript (.jsonl) into readable Markdown plus the screenshots it contains.

    export_transcript.py <session.jsonl> <outdir> <prefix>

Writes <outdir>/<prefix>-partNN.md (user and assistant text in full, tool calls as one-line summaries,
tool output cut to a short head, thinking blocks left out) and <outdir>/screenshots/<prefix>-NNN.<ext> for
every embedded image.  Pre-compaction summaries ("This session is being continued ...") are kept: they
are the best condensed record of what was decided and done.
"""
import base64, json, os, re, sys

PART_MESSAGES = 700           # messages per Markdown part


CTRL = dict.fromkeys([c for c in range(32) if c not in (9, 10)] + [127])


def head(s, n):
    s = s.strip()
    return s if len(s) <= n else s[:n].rstrip() + ' ...'


def tool_summary(b):
    name, inp = b.get('name', '?'), b.get('input', {}) or {}
    if name in ('Bash', 'PowerShell'):
        return f"{name}: `{head(inp.get('command', ''), 400)}`".replace('\n', ' ⏎ ')
    if name in ('Read', 'Edit', 'Write'):
        return f"{name}: {inp.get('file_path', '')}"
    if name == 'Agent':
        return f"Agent: {inp.get('description', '')}"
    return f"{name}: {head(json.dumps(inp, ensure_ascii=False), 200)}"


def main():
    src, outdir, prefix = sys.argv[1:4]
    shots = os.path.join(outdir, 'screenshots')
    os.makedirs(shots, exist_ok=True)
    seen, msgs, nimg = set(), [], 0
    index = []                                   # (image file, timestamp, who, context)
    for line in open(src, encoding='utf-8'):
        try:
            o = json.loads(line)
        except ValueError:
            continue
        if o.get('type') not in ('user', 'assistant') or not isinstance(o.get('message'), dict):
            continue
        uid = o.get('uuid')
        if uid in seen:
            continue
        seen.add(uid)
        ts = (o.get('timestamp') or '')[:19].replace('T', ' ')
        c = o['message'].get('content')
        parts = []

        def add_image(b, who):
            nonlocal nimg
            data = b.get('source', {}).get('data')
            if not data:
                return
            nimg += 1
            ext = {'image/png': 'png', 'image/jpeg': 'jpg', 'image/gif': 'gif'}.get(b['source'].get('media_type'), 'png')
            fn = f'{prefix}-{nimg:03d}.{ext}'
            with open(os.path.join(shots, fn), 'wb') as f:
                f.write(base64.b64decode(data))
            index.append((fn, ts, who))
            parts.append(f'![{fn}](screenshots/{fn})')

        if isinstance(c, str):
            parts.append(c.strip())
        elif isinstance(c, list):
            for b in c:
                if not isinstance(b, dict):
                    continue
                t = b.get('type')
                if t == 'text':
                    parts.append(b['text'].strip())
                elif t == 'image':
                    add_image(b, 'user' if o['type'] == 'user' else 'assistant')
                elif t == 'tool_use':
                    parts.append('› ' + tool_summary(b))
                elif t == 'tool_result':
                    r = b.get('content')
                    text = ''
                    if isinstance(r, str):
                        text = r
                    elif isinstance(r, list):
                        for bb in r:
                            if bb.get('type') == 'text':
                                text += bb['text']
                            elif bb.get('type') == 'image':
                                add_image(bb, 'tool output')
                    if text.strip():
                        parts.append('  ↳ ' + head(text, 240).replace('\n', ' ⏎ '))
        body = '\n\n'.join(p for p in parts if p)
        if not body:
            continue
        who = 'User' if o['type'] == 'user' else 'Assistant'
        if o['type'] == 'user' and isinstance(c, list) and all(isinstance(b, dict) and b.get('type') == 'tool_result' for b in c):
            who = 'Tool'
        msgs.append((ts, who, body.translate(CTRL).replace('\r', ''), bool(o.get('isSidechain'))))

    n = 0
    for start in range(0, len(msgs), PART_MESSAGES):
        n += 1
        chunk = msgs[start:start + PART_MESSAGES]
        fn = os.path.join(outdir, f'{prefix}-part{n:02d}.md')
        with open(fn, 'w', encoding='utf-8') as f:
            f.write(f'# {prefix} — part {n}  ({chunk[0][0]} … {chunk[-1][0]})\n\n')
            for ts, who, body, side in chunk:
                if who == 'Tool':
                    f.write(body + '\n\n')
                else:
                    f.write(f'## {who}{" (subagent)" if side else ""} · {ts}\n\n{body}\n\n')
    with open(os.path.join(outdir, f'{prefix}-images.tsv'), 'w', encoding='utf-8') as f:
        for fn, ts, who in index:
            f.write(f'{fn}\t{ts}\t{who}\n')
    print(f'{len(msgs)} messages, {n} parts, {nimg} images')


if __name__ == '__main__':
    main()
