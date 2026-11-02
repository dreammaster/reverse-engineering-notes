// Conversation scripts against the real CONVERSE.A / CONVERSE.B (skipped when absent).

#include "files/converse.h"

#include <cstdio>

using namespace Ultima6;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

int main() {
	Bytes a, b;
	if (!readFile("u6/game/CONVERSE.A", a) || !readFile("u6/game/CONVERSE.B", b)) {
		std::printf("skip (no game data)\n");
		return 0;
	}

	// Every actor that has a script: the script says it belongs to that actor.
	int loaded = 0, missing = 0;
	for (int actor = 0; actor < 0xE0; ++actor) {
		ConverseRef ref;
		CHECK(ConverseScript::locate(actor, 0, ref));
		ConverseScript script;
		if (!script.load(ref.second ? b : a, ref.item)) {
			++missing;
			continue;
		}
		++loaded;
		CHECK(script.actor() == actor);
		CHECK(!script.name().empty());
		CHECK(script.bytes()[script.codeStart()] == ConverseScript::SCONVERSE);
	}
	std::printf("%d scripts loaded, %d actors without one\n", loaded, missing);
	CHECK(loaded == 200); // 97 in CONVERSE.A, 103 in CONVERSE.B (the other items are absent)

	ConverseScript lb;
	CHECK(lb.load(a, 5) && lb.name() == "Lord British" && lb.description() == "the noble ruler of Britannia.\n*");

	// Structure: every script but one is understood to its end, jumps land on statements, and
	// Lord British's answers are found.
	int clean = 0, jumps = 0, inside = 0;
	for (int actor = 0; actor < 0xE0; ++actor) {
		ConverseRef r2;
		ConverseScript script;
		ConverseScript::locate(actor, 0, r2);
		if (!script.load(r2.second ? b : a, r2.item))
			continue;
		ConverseScan scan;
		script.scan(scan);
		clean += scan.clean;
		for (size_t k = 0; k < scan.items.size(); ++k) {
			if (scan.items[k].kind != ConverseScan::JUMP)
				continue;
			++jumps;
			for (const ConverseScan::Item &it : scan.items)
				if (it.kind != ConverseScan::TEXT && it.kind != ConverseScan::KEYWORDS && scan.items[k].target > it.offset &&
					scan.items[k].target < it.offset + it.length)
					++inside;
		}
	}
	std::printf("%d of 200 scripts scan cleanly; %d jumps, %d landing inside an operand\n", clean, jumps, inside);
	CHECK(clean >= 199);
	CHECK(inside * 200 < jumps); // under half a percent
	{
		ConverseScan scan;
		lb.scan(scan);
		CHECK(scan.keywords.size() == 55);
		CHECK(scan.keywords[0].keywords == "name" && scan.keywords[1].keywords == "rep" && scan.keywords[2].keywords == "job");
		CHECK(lb.bytes()[scan.keywords[0].answerOffset] == '"');
	}

	// Creatures: only three object types talk, using the scripts of actors 201-203.
	ConverseRef ref;
	CHECK(ConverseScript::locate(230, 0x175, ref) && ref.second && ref.item == 0x66);
	CHECK(!ConverseScript::locate(230, 0x100, ref));
	ConverseScript s201;
	CHECK(ConverseScript::locate(230, 0x17E, ref) && s201.load(b, ref.item) && s201.actor() == 99 + 0x67);

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
