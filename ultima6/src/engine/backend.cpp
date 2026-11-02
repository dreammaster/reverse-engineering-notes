#include "engine/backend.h"

namespace Ultima6 {

HeadlessBackend::HeadlessBackend(const std::string &script) : _pos(0), _clock(0) {
	for (size_t i = 0; i < script.size(); ++i) {
		char c = script[i];
		if (c == '{') {
			size_t end = script.find('}', i);
			if (end == std::string::npos)
				break;
			std::string name = script.substr(i + 1, end - i - 1);
			if (name == "up")
				_keys.push_back(KEY_UP);
			else if (name == "down")
				_keys.push_back(KEY_DOWN);
			else if (name == "left")
				_keys.push_back(KEY_LEFT);
			else if (name == "right")
				_keys.push_back(KEY_RIGHT);
			else if (name == "enter")
				_keys.push_back(KEY_ENTER);
			else if (name == "esc")
				_keys.push_back(KEY_ESC);
			else if (name == "space")
				_keys.push_back(KEY_SPACE);
			else if (name == "bs")
				_keys.push_back(KEY_BACKSPACE);
			i = end;
		} else {
			_keys.push_back((unsigned char)c);
		}
	}
}

int HeadlessBackend::pollKey() {
	return _pos < _keys.size() ? _keys[_pos++] : KEY_NONE;
}

int HeadlessBackend::waitKey() {
	return _pos < _keys.size() ? _keys[_pos++] : KEY_QUIT;
}

std::string HeadlessBackend::parseKeysDoc() {
	return "plain characters and {up} {down} {left} {right} {enter} {esc} {space} {bs}";
}

} // namespace Ultima6
