/* What the game logic needs from the platform: text output, waiting, sounds, dialogs and all the drawing the original's logic triggers.
 *
 * The logic (src/xanth/logic, game.cpp) calls these as the original calls its screen, window, mouse and sound routines. A `Ui` implementation draws and plays;
 * the headless one used by the tests only records what was asked (`RecordingUi`), which is what the oracle of engine/host records too.
 *
 * Functions of the original that only draw or play and that are not understood yet are reached through `other(name, args)`, so the logic that calls them
 * stays complete; each is replaced by a named member when its meaning is known.
 */
#ifndef XANTH_UI_H
#define XANTH_UI_H

#include <cstdio>

#include "common/types.h"

namespace Xanth {

class Ui {
public:
	virtual ~Ui() {}

	/** Prints text in the text window (Font_WriteMessage); the message has been formatted already. Returns what the original returns (see Game::message). */
	virtual int printText(const std::string &text) = 0;
	/** Prints one character (wputchar). */
	virtual int putChar(int ch) = 0;
	/** Draws a text box (show_text_box). */
	virtual int showTextBox(const std::string &text) = 0;
	/** Plays a digital sound effect file (realsound_play). */
	virtual int playSound(const std::string &file) = 0;
	/** Waits for `ticks` timer ticks or a click (wait_ticks_or_click). */
	virtual int waitTicksOrClick(int ticks) = 0;
	/** Asks a yes/no question; true for yes (show_yes_no_dialog). */
	virtual bool askYesNo(const std::string &question) = 0;

	// ---- conversations (Conversation_Show)
	/** Starts a conversation scene: the picture (file << 8 | entry) in a frame on the marble background. */
	virtual void conversationBegin(int picNum) { (void)picNum; }
	/** One line of speech (shown to the right of the picture); `wait`: wait for a click before going on. */
	virtual void conversationLine(const std::string &text, bool wait) { (void)text; (void)wait; }
	/** A menu of replies below the picture; `initial` is the entry that starts highlighted; returns the entry chosen. */
	virtual int conversationMenu(const std::vector<std::string> &options, int initial) { (void)options; return initial < 0 ? 0 : initial; }
	/** The end of the conversation: the scene is taken down and the game screen comes back. */
	virtual void conversationEnd() {}

	/** A routine of the original that draws, animates or plays; `name` is its name in the original (sub_XXXXX where nothing better is known), `args` the words it was called with. */
	virtual int other(const char *name, const intptr_t *args, int count) = 0;
};

/** A Ui that does nothing and returns 0 from everything - the behaviour of the oracle's stubs - while keeping a log of the text-producing calls. */
class RecordingUi : public Ui {
public:
	std::vector<std::string> log;
	std::vector<std::string> others;       // names of the routines reached through other(), in call order

	virtual int printText(const std::string &text) { log.push_back("Font_WriteMessage|" + text); return 0; }
	virtual int putChar(int ch) {
		std::string s("wputchar|");
		if (ch >= 0x20 && ch < 0x7F)
			s += (char)ch;
		log.push_back(s);
		return 0;
	}
	virtual int showTextBox(const std::string &text) { log.push_back("show_text_box|" + text); return 0; }
	virtual int playSound(const std::string &file) { log.push_back("realsound_play|" + file); return 0; }
	virtual int waitTicksOrClick(int) { return 0; }
	virtual bool askYesNo(const std::string &) { return false; }
	virtual int other(const char *name, const intptr_t *, int) { others.push_back(name); return 0; }
	virtual void conversationBegin(int picNum) { char b[32]; snprintf(b, sizeof(b), "conversation|%X", picNum); log.push_back(b); }
	virtual void conversationLine(const std::string &text, bool) { log.push_back("conversation_line|" + text); }
	virtual int conversationMenu(const std::vector<std::string> &options, int initial) {
		for (size_t i = 0; i < options.size(); i++)
			log.push_back("conversation_option|" + options[i]);
		return initial < 0 ? 0 : initial;
	}
};

} // namespace Xanth

#endif
