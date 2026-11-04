#include "ultima6/conversation.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

enum {
	OP_GT = 0x81, OP_GE = 0x82, OP_LT = 0x83, OP_LE = 0x84, OP_NE = 0x85, OP_EQ = 0x86,
	OP_ADD = 0x90, OP_SUB = 0x91, OP_MUL = 0x92, OP_DIV = 0x93, OP_LOR = 0x94, OP_LAND = 0x95,
	OP_CANCARRY = 0x9A, OP_WEIGHT = 0x9B, OP_HORSED = 0x9D, OP_HASOBJ = 0x9F, OP_RAND = 0xA0,
	OP_EVAL = 0xA7, OP_FLAG = 0xAB, OP_VAR = 0xB2, OP_SVAR = 0xB3, OP_DATA = 0xB4,
	OP_OBJCOUNT = 0xBB, OP_INPARTY = 0xC6, OP_OBJINPARTY = 0xC7, OP_JOIN = 0xCA, OP_LEAVE = 0xCC,
	OP_NPCNEARBY = 0xD7, OP_WOUNDED = 0xDA, OP_POISONED = 0xDC, OP_NPC = 0xDD,
	OP_EXP = 0xE0, OP_LVL = 0xE1, OP_STR = 0xE2, OP_INT = 0xE3, OP_DEX = 0xE4,
	OP_INDEXOF = 0xB7, OP_ENDDATA = 0xB8,
	// commands
	C_HORSE = 0x9C, C_SLEEP = 0x9E, C_IF = 0xA1, C_ENDIF = 0xA2, C_ELSE = 0xA3, C_SETF = 0xA4, C_CLEARF = 0xA5,
	C_DECL = 0xA6, C_ASSIGN = 0xA8, C_JUMP = 0xB0, C_DPRINT = 0xB5, C_BYE = 0xB6, C_NEW = 0xB9, C_DELETE = 0xBA,
	C_INVENTORY = 0xBE, C_PORTRAIT = 0xBF, C_ADDKARMA = 0xC4, C_SUBKARMA = 0xC5, C_GIVE = 0xC9, C_WAIT = 0xCB,
	C_WORKTYPE = 0xCD, C_RESURRECT = 0xD6, C_SETNAME = 0xD8, C_HEAL = 0xD9, C_CURE = 0xDB,
	C_ENDANSWER = 0xEE, C_KEYWORDS = 0xEF, C_SLOOK = 0xF1, C_SCONVERSE = 0xF2, C_SPREFIX = 0xF3, C_ANSWER = 0xF6,
	C_ASK = 0xF7, C_ASKC = 0xF8, C_INPUTSTR = 0xF9, C_INPUT = 0xFB, C_INPUTNUM = 0xFC, C_SIDENT = 0xFF
};

Conversation::Conversation(const Bytes &script, int npc, ConversationHost &host) : _script(script), _npc(npc), _host(host) {
}

void Conversation::setVar(int v, uint32_t value) {
	if (v <= VAR_LAST)
		_vars[v] = value;
}

void Conversation::setSVar(int v, const std::string &value) {
	if (v <= VAR_LAST)
		_svars[v] = value;
}

uint32_t Conversation::read() {
	if (_pos < _script.size())
		return _script[_pos++];
	++_pos;
	return 0;
}
uint32_t Conversation::read2() {
	uint32_t v = read();
	return v | (read() << 8);
}
uint32_t Conversation::read4() {
	uint32_t v = read2();
	return v | (read2() << 16);
}

bool Conversation::isPrint(uint32_t c) {
	return c == 0x0A || (c >= 0x20 && c <= 0x7A) || c == 0x7E || c == 0x7B;
}
bool Conversation::isDataSize(uint32_t c) {
	return c == 0xD3 || c == 0xD2 || c == 0xD4;
}
bool Conversation::isValOp(uint32_t c) {
	switch (c) {
	case OP_GT: case OP_GE: case OP_LT: case OP_LE: case OP_NE: case OP_EQ:
	case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_LOR: case OP_LAND:
	case OP_CANCARRY: case OP_WEIGHT: case OP_HORSED: case OP_HASOBJ: case OP_RAND: case OP_EVAL:
	case OP_FLAG: case OP_VAR: case OP_SVAR: case OP_DATA: case OP_INDEXOF: case OP_OBJCOUNT:
	case OP_INPARTY: case OP_OBJINPARTY: case OP_JOIN: case OP_LEAVE:
	case OP_NPCNEARBY: case OP_WOUNDED: case OP_POISONED: case OP_NPC:
	case OP_EXP: case OP_LVL: case OP_STR: case OP_INT: case OP_DEX:
		return true;
	default:
		return false;
	}
}
bool Conversation::isCtrl(uint32_t c) {
	return (c >= 0xA1 || c == 0x9C || c == 0x9E) && !isValOp(c) && !isDataSize(c);
}

const std::string &Conversation::rstr(uint32_t i) const {
	static const std::string empty;
	return i < _rstrings.size() ? _rstrings[i] : empty;
}
uint32_t Conversation::addRstr(const std::string &s) {
	_rstrings.push_back(s);
	return (uint32_t)_rstrings.size() - 1;
}
void Conversation::setYString(const std::string &s) {
	_ystring = s;
	_svars[VAR_YSTRING] = s;
}

uint32_t Conversation::pop(std::vector<TypedValue> &st) {
	if (st.empty())
		return 0;
	uint32_t v = st.back().val;
	st.pop_back();
	return v;
}
Conversation::TypedValue Conversation::popTyped(std::vector<TypedValue> &st) {
	TypedValue v = {0, 0};
	if (!st.empty()) {
		v = st.back();
		st.pop_back();
	}
	return v;
}

// ---- output ----

void Conversation::emit(const std::string &text) {
	_pendingOut += text;
	flushOutput();
}

void Conversation::flushOutput() {
	if (_pendingOut.empty())
		return;
	size_t star = _pendingOut.find('*');
	if (star == std::string::npos) {
		_host.print(_pendingOut);
		_pendingOut.clear();
		return;
	}
	_host.print(_pendingOut.substr(0, star + 1));
	_pendingOut.erase(0, star + 1);
	block(NEED_KEY);
}

void Conversation::block(State s) {
	_waiting = true;
	_state = s;
}

// APPROX(35): not the original's exact behaviour, see docs/approximations.md
std::string Conversation::format(const std::string &in) {
	std::string out;
	size_t i = 0, len = in.size();
	while (i < len) {
		char c = in[i];
		switch (c) {
		case '$':
		case '#': {
			char k = i + 1 < len ? in[i + 1] : 0;
			if (c == '$' && k == 'G')
				out += _host.genderTitle();
			else if (c == '$' && k == 'N')
				out += _name;
			else if (c == '$' && k == 'P')
				out += _host.playerName();
			else if (c == '$' && k == 'T')
				out += _host.timeOfDay();
			else if (c == '$' && k == 'Y')
				out += _ystring;
			else if (c == '$' && k == 'Z')
				out += _svars[VAR_INPUT];
			else if (c == '$' && std::isdigit((unsigned char)k))
				out += _svars[std::min<long>(std::strtol(&in[i + 1], nullptr, 10), VAR_LAST)];
			else if (c == '#' && std::isdigit((unsigned char)k)) {
				_lastValue = var((int)std::strtol(&in[i + 1], nullptr, 10));
				out += std::to_string(_lastValue);
			} else {
				out += c;
				if (k)
					out += k;
			}
			i += 2;
			break;
		}
		case '{':
			++i;
			break;
		case '~': // speech cue
			if (i + 3 <= len) {
				++i;
				for (++i; i < len && std::isdigit((unsigned char)in[i]);)
					++i;
			}
			break;
		case '/':
		case '\\': { // singular / plural ending
			bool show = (_lastValue == 1 && c == '/') || (_lastValue != 1 && c == '\\');
			++i;
			size_t count = 0;
			while (i + count < len && in[i + count] >= 'a' && in[i + count] <= 'z')
				++count;
			if (show)
				out.append(in, i, count);
			i += count;
			break;
		}
		case '+': // a cue between plusses, skipped in the original game
			if (i + 3 <= len) {
				++i;
				while (i < len && in[i] != '+')
					++i;
				++i;
			} else {
				out += c;
				++i;
			}
			break;
		default:
			out += c;
			++i;
			break;
		}
	}
	return out;
}

// ---- reading statements ----

void Conversation::addText() {
	do {
		_text.push_back((char)read());
	} while (!overflow() && isPrint(peek()));
}

void Conversation::collectInput() {
	uint32_t c = peek();
	if (c == C_JUMP) {
		uint32_t opc = read();
		_in.push_back({opc, 0});
		_in.push_back({read4(), 0});
		return;
	}
	if (c == C_SIDENT) {
		uint32_t opc = read();
		_in.push_back({opc, 0});
		_in.push_back({read(), 0});
		addText();
		return;
	}
	if (c == C_KEYWORDS || c == C_ASKC || c == C_SLOOK) {
		_in.push_back({read(), 0});
		addText();
		return;
	}
	size_t arg = 1;
	do {
		if (isPrint(peek()) && (overflow(1) || !isValOp(peek(1))))
			break;
		if (peek() == OP_EVAL) {
			read();
			if (running())
				eval(arg++);
		} else {
			uint32_t v = read();
			uint8_t size = 0;
			if (v == 0xD3) {
				size = 0xD3;
				v = read();
			} else if (v == 0xD2) {
				size = 0xD2;
				v = read4();
			} else if (v == 0xD4) {
				size = 0xD4;
				v = read2();
			}
			_in.push_back({v, size});
		}
	} while (!overflow() && !isCtrl(peek()));
}

void Conversation::eval(size_t from) {
	std::vector<TypedValue> st;
	for (size_t v = from; v < _in.size(); ++v) {
		TypedValue a = {OP_VAR, _in[v].v};
		st.push_back(a);
		if (isValOp(a.val) && !_in[v].size)
			evop(st);
	}
	_in.resize(from);
	if (st.empty()) {
		_in.push_back({0, 0});
		return;
	}
	for (const TypedValue &t : st)
		_in.push_back({t.val, 0});
}

void Conversation::doFrame(uint32_t c) {
	switch (c) {
	case C_IF:
		_frames.push_back({C_IF, running(), 0});
		break;
	case C_ENDIF:
		if (!_frames.empty())
			_frames.pop_back();
		break;
	case C_KEYWORDS:
		if (_frames.empty() || _frames.back().startCode != C_KEYWORDS)
			_frames.push_back({C_KEYWORDS, running(), 0});
		break;
	case C_ENDANSWER:
		if (!_frames.empty())
			_frames.pop_back();
		break;
	default:
		break;
	}
	if (c != 0 && top() && c == top()->breakCode)
		top()->run = !top()->run;
}

void Conversation::exec() {
	uint32_t first = _in.empty() ? 0 : _in[0].v;
	doFrame(first);
	if (running()) {
		if (!_in.empty()) {
			std::vector<TypedValue> st;
			while (!_in.empty()) {
				st.push_back({OP_VAR, _in.back().v});
				_in.pop_back();
			}
			op(st);
		} else if (!_text.empty()) {
			emit(format(_text));
		}
	}
	_in.clear();
	_text.clear();
}

void Conversation::step() {
	while (!_waiting && !overflow() && !_stopped) {
		if (isPrint(peek())) {
			addText();
		} else if (isCtrl(peek())) {
			_inStart = _pos;
			collectInput();
		} else {
			emit("[Tried to print a control char.]\n");
			read();
			continue;
		}
		exec();
	}
	if (overflow() && !_stopped && !_waiting) {
		emit("\n[EOF]\n");
		_stopped = true;
		_waiting = true;
	}
}

Conversation::State Conversation::run() {
	if (_stopped)
		return ENDED;
	if (!_waiting) {
		flushOutput();
		if (!_waiting)
			step();
	}
	if (_stopped)
		return ENDED;
	return _state;
}

void Conversation::giveKey() {
	if (_state != NEED_KEY || !_waiting)
		return;
	_host.print("\b"); // the star disappears
	_waiting = false;
	flushOutput();
}

void Conversation::giveChar(int c) {
	if (_state != NEED_CHAR || !_waiting)
		return;
	std::string s(1, (char)c);
	_host.print("\n\n");
	_lastInput = s;
	if (_declType == 0xB2)
		setVar(_declVar, (uint32_t)std::strtol(s.c_str(), nullptr, 10));
	else if (_declType == 0xB3 && _declVar <= VAR_LAST)
		_svars[_declVar] = s;
	_svars[VAR_INPUT] = s;
	_waiting = false;
}

void Conversation::giveLine(const std::string &raw) {
	if (_state != NEED_LINE || !_waiting)
		return;
	std::string line = raw;
	_host.print("\n\n");
	if (line.empty())
		line = "bye";
	else if (line.size() == 4 && std::tolower((unsigned char)line[0]) == 'l' && std::tolower((unsigned char)line[1]) == 'o' && std::tolower((unsigned char)line[2]) == 'o' && std::tolower((unsigned char)line[3]) == 'k') { // _stricmp(input, "look")
		_host.print("You see " + _description + "\n");
		_pos = _askPos; // back to the ASK that wanted the line
		_waiting = false;
		return;
	}
	_lastInput = line;
	if (_declType == 0xB2)
		setVar(_declVar, (uint32_t)std::strtol(line.c_str(), nullptr, 10));
	else if (_declType == 0xB3 && _declVar <= VAR_LAST)
		_svars[_declVar] = line;
	_svars[VAR_INPUT] = line;
	_waiting = false;
}

// sub_1ECB6: a keyword matches when it is the start of one of the words the player typed (split at spaces), compared without regard to case and with ? in the keyword standing for any
// character; a comma separates the keywords of a list (sub_209B1), and "*" takes anything. (The list of words of findDbString has its own limit of four letters.)
bool Conversation::matchKeywords(const std::string &keys, const std::string &input) const {
	if (keys == "*")
		return true;
	std::vector<std::string> words;
	size_t p = 0;
	while (p < input.size()) {
		while (p < input.size() && input[p] == ' ')
			++p;
		size_t q = input.find(' ', p);
		if (p >= input.size())
			break;
		words.push_back(input.substr(p, q == std::string::npos ? std::string::npos : q - p));
		p = q == std::string::npos ? input.size() : q;
	}
	size_t start = 0;
	while (start <= keys.size()) {
		size_t comma = keys.find(',', start);
		std::string key = keys.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
		for (const std::string &word : words) {
			size_t i = 0;
			for (; i < key.size(); ++i) {
				int c = i < word.size() ? std::tolower((unsigned char)word[i]) : 0;
				if (std::tolower((unsigned char)key[i]) != c && key[i] != '?')
					break;
			}
			if (i == key.size())
				return true;
		}
		if (comma == std::string::npos)
			break;
		start = comma + 1;
	}
	return false;
}

// ---- data sections ----

std::string Conversation::dbString(uint32_t loc, uint32_t index) const {
	if (loc >= _script.size())
		return "";
	size_t p = loc;
	for (uint32_t e = 0; e < index; ++e) {
		if (p >= _script.size() || _script[p] == OP_ENDDATA)
			return "";
		while (p < _script.size() && isPrint(_script[p++])) {
		}
	}
	std::string s;
	while (p < _script.size() && isPrint(_script[p]))
		s.push_back((char)_script[p++]);
	return s;
}

uint32_t Conversation::dbInteger(uint32_t loc, uint32_t index) const {
	size_t p = (size_t)loc + 2 * index;
	return p + 1 < _script.size() ? (uint32_t)(_script[p] | (_script[p + 1] << 8)) : 0;
}

void Conversation::setDbInteger(uint32_t loc, uint32_t index, uint32_t value) {
	size_t p = (size_t)loc + 2 * index;
	if (p + 1 < _script.size()) {
		_script[p] = value & 0xFF;
		_script[p + 1] = (value >> 8) & 0xFF;
	}
}

uint32_t Conversation::findDbString(uint32_t loc, const std::string &s) const {
	size_t p = loc;
	uint32_t i = 0;
	while (p < _script.size() && _script[p] != OP_ENDDATA) {
		if (isPrint(_script[p])) {
			std::string item;
			while (p < _script.size() && isPrint(_script[p]))
				item.push_back((char)_script[p++]);
			++p;
			if (item.size() > 4)
				item.erase(4);
			if (matchKeywords(item, s))
				return i;
		} else
			++p;
		++i;
	}
	return i;
}

// ---- value operators ----

bool Conversation::evop(std::vector<TypedValue> &st) {
	bool ok = true;
	uint32_t v[4];
	TypedValue out = {OP_VAR, 0};
	uint32_t code = pop(st);
	switch (code) {
	case OP_GT:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = v[0] > v[1];
		break;
	case OP_GE:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = v[0] >= v[1];
		break;
	case OP_LT:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = v[0] < v[1];
		break;
	case OP_LE:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = v[0] <= v[1];
		break;
	case OP_NE:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = v[0] != v[1];
		break;
	case OP_EQ: {
		TypedValue a = popTyped(st), b = popTyped(st);
		if (a.type == OP_VAR) {
			out.val = a.val == b.val;
		} else {
			const std::string &x = rstr(a.val), &y = rstr(b.val);
			bool same = x.size() == y.size();
			for (size_t i = 0; same && i < x.size(); ++i)
				same = std::tolower((unsigned char)x[i]) == std::tolower((unsigned char)y[i]);
			out.val = same;
		}
		break;
	}
	case OP_ADD:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = v[0] + v[1];
		break;
	case OP_SUB:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = v[0] - v[1];
		break;
	case OP_MUL:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = v[0] * v[1];
		break;
	case OP_DIV:
		v[1] = pop(st);
		v[0] = pop(st);
		if (v[1] == 0) {
			ok = false;
			_stopped = true;
			_waiting = true;
		} else
			out.val = v[0] / v[1];
		break;
	case OP_LOR:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = v[0] || v[1];
		break;
	case OP_LAND:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = v[0] && v[1];
		break;
	case OP_CANCARRY:
		out.val = (uint32_t)_host.maxCarry((int)pop(st));
		break;
	case OP_WEIGHT:
		v[1] = pop(st); // quantity
		v[0] = pop(st); // object
		out.val = (uint32_t)_host.objectWeight((int)v[0], (int)v[1]);
		break;
	case OP_HORSED:
		out.val = _host.horsed(npcNumber(pop(st)));
		break;
	case OP_HASOBJ:
		v[2] = pop(st); // quality
		v[1] = pop(st); // object
		v[0] = pop(st); // npc
		out.val = _host.hasObject(npcNumber(v[0]), (int)v[1], (int)v[2], v[2] != 0);
		break;
	case OP_RAND:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = v[0] >= v[1] ? v[0] : (uint32_t)_host.random((int)v[0], (int)v[1]);
		break;
	case OP_FLAG:
		v[1] = pop(st);
		v[0] = pop(st);
		if (v[1] <= 7)
			out.val = (_host.talkFlags(npcNumber(v[0])) >> v[1]) & 1;
		break;
	case OP_VAR:
		out.val = var((int)pop(st));
		break;
	case OP_SVAR: {
		uint32_t n = pop(st);
		out.val = addRstr(n <= VAR_LAST ? _svars[n] : std::string());
		out.type = OP_SVAR;
		break;
	}
	case OP_DATA:
		v[1] = pop(st); // index
		v[0] = pop(st); // location
		if (_declType == 0xB3) {
			out.val = addRstr(dbString(v[0], v[1]));
			out.type = OP_SVAR;
		} else
			out.val = dbInteger(v[0], v[1]);
		break;
	case OP_INDEXOF:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = findDbString(v[0], v[1] <= VAR_LAST ? _svars[v[1]] : std::string());
		break;
	case OP_OBJCOUNT:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = (uint32_t)_host.countObject(npcNumber(v[0]), (int)v[1]);
		break;
	case OP_INPARTY:
		out.val = _host.inParty(npcNumber(pop(st)));
		break;
	case OP_OBJINPARTY: {
		v[1] = pop(st); // quality
		v[0] = pop(st); // object
		int who = _host.whoHasObject((int)v[0], (int)v[1]);
		out.val = who < 0 ? 0x8001 : (uint32_t)who;
		break;
	}
	case OP_JOIN:
		out.val = (uint32_t)_host.joinParty(npcNumber(pop(st)));
		break;
	case OP_LEAVE:
		out.val = (uint32_t)_host.leaveParty(npcNumber(pop(st)));
		break;
	case OP_NPCNEARBY:
		out.val = _host.npcNearby(npcNumber(pop(st)));
		break;
	case OP_WOUNDED:
		out.val = _host.wounded(npcNumber(pop(st)));
		break;
	case OP_POISONED:
		out.val = _host.poisoned(npcNumber(pop(st)));
		break;
	case OP_NPC:
		v[1] = pop(st);
		v[0] = pop(st);
		out.val = (uint32_t)_host.partyMember((int)v[0]);
		break;
	case OP_EXP:
	case OP_LVL:
	case OP_STR:
	case OP_INT:
	case OP_DEX:
		v[1] = pop(st); // amount to add
		v[0] = pop(st);
		out.val = (uint32_t)_host.adjustStat(npcNumber(v[0]), (int)(code - OP_EXP), (int)v[1]);
		break;
	default:
		ok = false;
	}
	st.push_back(out);
	return ok;
}

// ---- commands ----

bool Conversation::op(std::vector<TypedValue> &st) {
	uint32_t v[4] = {0, 0, 0, 0};
	uint32_t code = pop(st);
	switch (code) {
	case C_SLEEP:
		_host.sleep();
		break;
	case C_HORSE:
		_host.mount(npcNumber(pop(st)));
		break;
	case C_IF:
		if (top()) {
			top()->breakCode = C_ELSE;
			top()->run = pop(st) != 0;
		}
		break;
	case C_ENDIF:
	case C_ELSE:
		break;
	case C_SETF:
		v[0] = npcNumber(pop(st));
		v[1] = pop(st);
		_host.setTalkFlags((int)v[0], _host.talkFlags((int)v[0]) | (1 << (v[1] & 7)));
		break;
	case C_CLEARF:
		v[0] = npcNumber(pop(st));
		v[1] = pop(st);
		_host.setTalkFlags((int)v[0], _host.talkFlags((int)v[0]) & ~(1 << (v[1] & 7)));
		break;
	case C_DECL:
		_dbLvar = false;
		v[0] = pop(st); // variable
		v[1] = pop(st); // type
		if (!st.empty()) {
			TypedValue t = st.back();
			if (t.val == 0xB2 || t.val == 0xB4) {
				if (t.val == 0xB2) { // DB1[#2] = ...
					pop(st);
					pop(st);
					_dbLvar = true;
					_dbLoc = v[0];
					_dbOffset = var((int)v[1]);
				} else { // DB1[0] = ...
					pop(st);
					_dbLvar = true;
					_dbLoc = v[0];
					_dbOffset = v[1];
				}
			}
		}
		_declVar = (uint8_t)v[0];
		_declType = (uint8_t)v[1];
		break;
	case C_ASSIGN:
	case C_SETNAME:
		v[0] = pop(st);
		if (code == C_ASSIGN) {
			if (_dbLvar)
				setDbInteger(_dbLoc, _dbOffset, v[0]);
			else if (_declVar <= VAR_LAST) {
				if (_declType == 0xB3)
					_svars[_declVar] = rstr(v[0]);
				else if (_declType == 0xB2)
					_vars[_declVar] = v[0];
			} else
				emit("[Illegal variable]\n");
		} else {
			int n = npcNumber(v[0]);
			bool member = false;
			std::string nm = _host.partyName(n, member);
			setYString(member ? nm : _host.scriptName(n));
		}
		_declVar = _declType = 0;
		break;
	case C_JUMP:
		v[0] = pop(st);
		_pos = v[0];
		_frames.clear();
		break;
	case C_DPRINT:
		v[0] = pop(st); // location
		v[1] = pop(st); // index
		emit(format(dbString(v[0], v[1])));
		break;
	case C_BYE:
		_stopped = true;
		_waiting = true;
		break;
	case C_NEW:
		v[0] = pop(st); // npc
		v[1] = pop(st); // object
		v[2] = pop(st); // quality
		v[3] = pop(st); // quantity
		_host.newObject(npcNumber(v[0]), (int)v[1], (int)v[2], (int)v[3]);
		break;
	case C_DELETE:
		v[0] = pop(st);
		v[1] = pop(st);
		v[2] = pop(st);
		v[3] = pop(st);
		_host.deleteObject(npcNumber(v[0]), (int)v[1], (int)v[2], (int)v[3]);
		break;
	case C_GIVE:
		v[0] = pop(st); // object
		v[1] = pop(st); // quality
		v[2] = pop(st); // from
		v[3] = pop(st); // to
		_host.giveObject((int)v[0], (int)v[1], npcNumber(v[2]), npcNumber(v[3]));
		break;
	case C_ADDKARMA:
		_host.addKarma((int)pop(st));
		break;
	case C_SUBKARMA:
		_host.subtractKarma((int)pop(st));
		break;
	case C_RESURRECT:
		_host.resurrect(npcNumber(pop(st)));
		break;
	case C_HEAL:
		_host.heal(npcNumber(pop(st)));
		break;
	case C_CURE:
		_host.cure(npcNumber(pop(st)));
		break;
	case C_WORKTYPE:
		v[0] = pop(st);
		v[1] = pop(st);
		_host.setWorktype(npcNumber(v[0]), (int)v[1]);
		break;
	case C_INVENTORY:
		emit("!inventory\n");
		break;
	case C_PORTRAIT:
		_host.showPortrait(npcNumber(pop(st)));
		break;
	case C_WAIT:
		emit("*");
		break;
	case C_ENDANSWER:
		break;
	case C_KEYWORDS:
		if (_answer != ANSWER_DONE)
			_answer = matchKeywords(_text, _lastInput) ? ANSWER_YES : ANSWER_NO;
		break;
	case C_SIDENT:
		v[0] = pop(st);
		_name = _text;
		break;
	case C_SLOOK:
		_description = format(_text);
		emit("\n\nYou see " + _description + "\n");
		break;
	case C_SCONVERSE:
	case C_SPREFIX:
		break;
	case C_ANSWER:
		if (top()) {
			top()->breakCode = C_KEYWORDS;
			if (_answer == ANSWER_YES) {
				top()->run = true;
				_answer = ANSWER_DONE;
			} else
				top()->run = false;
		}
		break;
	case C_ASK:
		_answer = ANSWER_NO;
		_askPos = _inStart;
		_host.print("\nyou say:");
		_declVar = _declType = 0;
		block(NEED_LINE);
		break;
	case C_ASKC:
		_answer = ANSWER_NO;
		_allowed = _text;
		_declVar = _declType = 0;
		block(NEED_CHAR);
		break;
	case C_INPUTSTR:
	case C_INPUT:
		if (code == C_INPUTSTR)
			_answer = ANSWER_NO;
		v[0] = pop(st);
		v[1] = pop(st);
		_declVar = (uint8_t)v[0];
		_declType = (uint8_t)v[1];
		_allowed.clear();
		block(NEED_LINE);
		break;
	case C_INPUTNUM:
		v[0] = pop(st);
		v[1] = pop(st);
		_declVar = (uint8_t)v[0];
		_declType = (uint8_t)v[1];
		_allowed = "0123456789";
		block(NEED_LINE);
		break;
	default:
		emit("[Unknown command]\n");
		return false;
	}
	return true;
}

} // namespace Ultima6
} // namespace Ultima6
