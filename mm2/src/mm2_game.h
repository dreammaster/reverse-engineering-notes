/* Minimal game session: current map, party position, movement and event triggers.
 * This is the integration point of the data layer, the view and the event interpreter; the user
 * interface is left to the frontend (it reads `messages` and `locations`). */
#ifndef MM2_GAME_H
#define MM2_GAME_H

#include "mm2_events.h"
#include "mm2_files.h"
#include "mm2_treasure.h"

#define MM2_GAME_MAX_MSGS 16

typedef struct {
	char text[256];
	int opcode;   /* EV_MSG ... EV_MSG_FRAMED */
} Mm2GameMessage;

typedef struct {
	const Mm2Game *files;
	int map, x, y;
	char facing;
	uint8_t data[512];       /* walls then flags of the current map */
	Mm2Blob eventBlob;
	Mm2EventChunk events;
	int hasEvents;
	Mm2State state;
	Mm2Vm vm;
	Mm2EventHost host;
	/* outputs of the last step */
	Mm2GameMessage messages[MM2_GAME_MAX_MSGS];
	int nMessages;
	int locations[MM2_GAME_MAX_MSGS];   /* opcode 14 arguments */
	int nLocations;
	Mm2Treasure treasure;                /* placed by opcode 42 on the current spot */
	int treasureHere;
	int yesNo;                          /* answer given to Y/N prompts (opcodes 9, 10); default 1 */
	int fightRequested;                 /* opcode 18/19 seen */
	uint8_t fightMonsters[10];
	int pendingMap, pendingX, pendingY; /* teleport target (pendingMap < 0: none) */
} Mm2GameSession;

/* Human-readable name of an opcode 14 location code ("Inn", "Blacksmith" ...; "Entrance" for a map entrance). */
const char *mm2_location_name(int code);

/* Starts a session on `map` at (x, y).  Returns 0 on failure. */
int mm2_session_start(Mm2GameSession *s, const Mm2Game *files, int map, int x, int y, char facing);
void mm2_session_end(Mm2GameSession *s);

/* Connects the event interpreter to the party: character opcodes act on `roster` and the event variables live in its state
 * block (as in the saved game); opcode 33 edits the session's map. */
void mm2_session_attach(Mm2GameSession *s, Mm2Roster *roster);

/* Text of event message n (1-based) of the current map into out (cap bytes); returns its length or -1. */
int mm2_session_message(const Mm2GameSession *s, int n, char *out, size_t cap);

/* Turns: dir -1 left, +1 right.  Moves: returns 1 if the party moved (then triggers are run). */
void mm2_session_turn(Mm2GameSession *s, int dir);
int mm2_session_step(Mm2GameSession *s, int backwards);

/* Runs the trigger of the party's cell, if any.  Returns 1 if a script ran. */
int mm2_session_run_trigger(Mm2GameSession *s);

#endif
