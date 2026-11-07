// wxFileConfig (wxWidgets), the part that the plugin loader of TGameControl::InitScripts() uses: a configuration file of lines
// `key=value` (with groups in [brackets]) read from a file, or from the bytes of one (a file in a container), and Read() of a text.
// Written for this reconstruction on the C++ library: the names are those of wxWidgets. A value in quotes loses them; a line that starts with
// `;` or `#` is a comment; the keys without a group are those that Read("Key") finds.
#pragma once

#include <cstdio>
#include <map>
#include <string>

#include "WxStub.h"

class wxFileConfig {
public:
	wxFileConfig() = default;

	/** The bytes of a file. */
	wxFileConfig(const char *buffer, size_t size, bool /*useUtf8*/) {
		parse(std::string(buffer ? buffer : "", buffer ? size : 0));
	}

	/** A file on the disk (an unreadable one gives no keys). */
	wxFileConfig(const wxFileName &file, bool /*useUtf8*/) {
		std::FILE *handle = std::fopen(static_cast<const char *>(file.GetFullPath().mb_str()), "rb");

		if (!handle)
			return;

		std::string text;
		char block[4096];
		size_t count;

		while ((count = std::fread(block, 1, sizeof(block), handle)) > 0)
			text.append(block, count);
		std::fclose(handle);
		parse(text);
	}

	/** The value of `key` (in the group of its path: "Group/Key"), or `def` and false when there is none. */
	bool Read(const wxString &key, wxString *value, const wchar_t *def = nullptr) const {
		std::wstring wanted(key.wc_str());
		auto found = _entries.find(wanted);

		if (found == _entries.end()) {
			// a key that the file keeps in a group is found by its own name too (the first such one)
			std::wstring tail = L"/" + wanted;

			for (auto entry = _entries.begin(); entry != _entries.end(); ++entry) {
				if (entry->first.size() > tail.size() && entry->first.compare(entry->first.size() - tail.size(), tail.size(), tail) == 0) {
					found = entry;
					break;
				}
			}
		}
		if (found != _entries.end()) {
			*value = wxString(found->second.c_str());
			return true;
		}
		if (def)
			*value = wxString(def);
		return false;
	}

private:
	std::map<std::wstring, std::wstring> _entries;

	void parse(const std::string &text) {
		std::wstring group;
		size_t start = 0;

		while (start <= text.size()) {
			size_t end = text.find('\n', start);

			if (end == std::string::npos)
				end = text.size();

			std::string line = text.substr(start, end - start);

			start = end + 1;
			trim(line);
			if (line.empty() || line[0] == ';' || line[0] == '#')
				continue;
			if (line[0] == '[') {
				size_t close = line.find(']');

				group = widen(line.substr(1, close == std::string::npos ? std::string::npos : close - 1));
				if (!group.empty())
					group += L"/";
				continue;
			}

			size_t equals = line.find('=');

			if (equals == std::string::npos)
				continue;

			std::string key = line.substr(0, equals);
			std::string value = line.substr(equals + 1);

			trim(key);
			trim(value);
			if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
				value = value.substr(1, value.size() - 2);
			_entries[group + widen(key)] = widen(value);
		}
	}

	static void trim(std::string &text) {
		const char *space = " \t\r\n";
		size_t first = text.find_first_not_of(space);

		if (first == std::string::npos) {
			text.clear();
			return;
		}
		text = text.substr(first, text.find_last_not_of(space) - first + 1);
	}

	static std::wstring widen(const std::string &text) {
		wxString converted;

		toUTF(&converted, text.c_str());
		return std::wstring(converted.wc_str());
	}
};
