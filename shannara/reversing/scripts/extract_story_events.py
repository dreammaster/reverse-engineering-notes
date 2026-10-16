"""
Lists the story-changing calls made by each object handler (go_to_room?, start_encounter?, play_cutscene?,
party_add_member?, journal_add_entry?, add_score?, object_move?) with their constant arguments, joined with the
owning object ids from retail_object_handlers.tsv.  Input: analysis/retail_handler_digest.txt (extract_handler_digest.py).

    python extract_story_events.py        (run from reversing/scripts)
"""
import csv
import re

TARGETS = ("go_to_room?", "start_encounter?", "play_cutscene?", "party_add_member?", "journal_add_entry?", "add_score?", "object_move?")
owners = {}
for r in csv.reader(open("../analysis/retail_object_handlers.tsv", encoding="utf-8"), delimiter="\t"):
    if r[0].isdigit():
        owners["objh_%03d" % int(r[0])] = r[3]
out = open("../analysis/retail_story_events.tsv", "w", encoding="utf-8")
out.write("handler\tobjects(id:type)\tcall\targs\n")
n = 0
for blk in open("../analysis/retail_handler_digest.txt", encoding="utf-8").read().split("== ")[1:]:
    name, _, body = blk.partition("\n")
    for l in body.split("\n"):
        m = re.match(r"\s+(\S+)\((.*)\)", l)
        if m and m.group(1) in TARGETS:
            out.write("%s\t%s\t%s\t%s\n" % (name, owners.get(name, ""), m.group(1), m.group(2)))
            n += 1
print(n, "events")
