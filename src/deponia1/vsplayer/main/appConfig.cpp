#include "vsplayer/main/appConfig.h"

#include <cwctype>
#include <fstream>
#include <string>

#include "AppGlobals.h"

static bool equalNoCase(const std::wstring &a, const wchar_t *b) {
	size_t length = std::wcslen(b);

	if (a.size() != length)
		return false;

	for (size_t i = 0; i < length; i++) {
		if (std::towlower(a[i]) != std::towlower(b[i]))
			return false;
	}

	return true;
}

static bool isDigit(wchar_t c) {
	return c >= L'0' && c <= L'9';
}

// Confirmed (asm lines 540151-540420)
bool ConvertStringToSize(const wxString &text, wxSize &size) {
	const std::wstring string = text.ToStdWstring();
	size_t position = 0;

	while (position < string.size() && isDigit(string[position]))
		position++;

	size_t widthDigits = position;

	if (widthDigits == 0 || position >= string.size() || string[position] != L'x')
		return false;

	position++;

	size_t heightStart = position;

	while (position < string.size() && isDigit(string[position]))
		position++;

	if (position < string.size() || position == heightStart)
		return false;

	long width = std::stol(string.substr(0, widthDigits));
	long height = std::stol(string.substr(heightStart));

	size.Set(static_cast<int>(width), static_cast<int>(height));
	return true;
}

// A number from 0 to 100 (the volumes and the brightness); true when the text is one.
static bool percent(const wxString &text, long &value) {
	long number;

	if (!text.ToLong(&number, 10) || number < 0 || number > 100)
		return false;

	value = number;
	return true;
}

// Confirmed (asm lines 500424-503036)
bool LoadConfigFile(const wxString &path, const TConfigTargets &targets) {
	std::ifstream stream(static_cast<const char *>(path.mb_str()));

	if (!stream.is_open()) {
		if (wxLog::loglevel > 1)
			wxLog::logexpanded(L"No config.ini in %s", path.wc_str());

		return false;
	}

	if (wxLog::loglevel > 1)
		wxLog::logexpanded(L"Loading from %s", path.wc_str());

	bool lineWithoutEquals = false;
	std::string narrow;

	while (std::getline(stream, narrow)) {
		if (!narrow.empty() && narrow.back() == '\r')
			narrow.pop_back();

		if (narrow.empty())
			continue;

		wxString lineText;

		toUTF(&lineText, narrow.c_str());

		const std::wstring line = lineText.ToStdWstring();

		if (line[0] == L'\'' || line[0] == L'#')
			continue;

		size_t equals = line.find(L'=');

		if (equals == std::wstring::npos) {
			lineWithoutEquals = true;
			continue;
		}

		// the name ends at the first space before the `=` or at the `=`
		size_t space = line.find(L' ');
		size_t nameEnd = (space == std::wstring::npos || space > equals) ? equals : space;
		const std::wstring name = line.substr(0, nameEnd);

		// the value is the word after the `=` (and the spaces)
		size_t valueStart = line.find_first_not_of(L' ', equals + 1);

		if (valueStart == std::wstring::npos)
			valueStart = equals + 1;

		size_t valueEnd = line.find(L' ', valueStart);

		if (valueEnd == std::wstring::npos)
			valueEnd = line.size();

		if (valueStart > line.size())
			valueStart = line.size();

		const wxString valueText(line.substr(valueStart, valueEnd - valueStart));
		const std::wstring value = valueText.ToStdWstring();
		long number;

		if (equalNoCase(name, L"Language")) {
			if (targets.language)
				*targets.language = valueText;
		} else if (equalNoCase(name, L"File")) {
			if (targets.file) {
				size_t quote = line.find(L'"');

				if (quote != std::wstring::npos) {
					// a path with spaces is in "": everything between the first and the last
					std::wstring rest = line.substr(quote + 1);
					size_t last = rest.rfind(L'"');

					*targets.file = wxString((last == std::wstring::npos) ? std::wstring() : rest.substr(0, last));
				} else {
					*targets.file = valueText;
				}
			}
		} else if (equalNoCase(name, L"MusicVolume")) {
			if (targets.musicVolume && percent(valueText, number))
				*targets.musicVolume = number;
		} else if (equalNoCase(name, L"SoundVolume")) {
			if (targets.soundVolume && percent(valueText, number))
				*targets.soundVolume = number;
		} else if (equalNoCase(name, L"SpeechVolume")) {
			if (targets.speechVolume && percent(valueText, number))
				*targets.speechVolume = number;
		} else if (equalNoCase(name, L"MovieVolume")) {
			if (targets.movieVolume && percent(valueText, number))
				*targets.movieVolume = number;
		} else if (equalNoCase(name, L"GlobalVolume")) {
			if (targets.globalVolume && percent(valueText, number))
				*targets.globalVolume = number;
		} else if (equalNoCase(name, L"Fullscreen")) {
			if (targets.fullscreen) {
				if (equalNoCase(value, L"Yes"))
					*targets.fullscreen = true;
				else if (equalNoCase(value, L"No"))
					*targets.fullscreen = false;
			}
		} else if (equalNoCase(name, L"Resizeable")) {
			if (targets.resizeable) {
				if (equalNoCase(value, L"Yes"))
					*targets.resizeable = true;
				else if (equalNoCase(value, L"No"))
					*targets.resizeable = false;
			}
		} else if (equalNoCase(name, L"Intro")) {
			if (targets.intro) {
				if (equalNoCase(value, L"Yes"))
					*targets.intro = true;
				else if (equalNoCase(value, L"No"))
					*targets.intro = false;
			}
		} else if (equalNoCase(name, L"Resolution")) {
			if (targets.resolution) {
				if (equalNoCase(value, L"Desktop")) {
					*targets.resolution = eResolution::kDesktop;
				} else if (equalNoCase(value, L"Game")) {
					*targets.resolution = eResolution::kGame;
				} else if (equalNoCase(value, L"Auto")) {
					*targets.resolution = eResolution::kAuto;
				} else {
					wxSize size;

					if (ConvertStringToSize(valueText, size)) {
						if (targets.resolutionSize)
							*targets.resolutionSize = size;

						*targets.resolution = eResolution::kSize;
					} else if (wxLog::loglevel > 0) {
						wxLog::logexpanded(L"Invalid value for parameter Resolution in config file.");
					}
				}
			}
		} else if (equalNoCase(name, L"UseTextureCompression")) {
			if (targets.textureCompression) {
				if (equalNoCase(value, L"Enabled"))
					*targets.textureCompression = true;
				else if (equalNoCase(value, L"Disabled"))
					*targets.textureCompression = false;
			}
		} else if (equalNoCase(name, L"UseTextureForWidescreen")) {
			if (targets.textureForWidescreen) {
				if (equalNoCase(value, L"Yes"))
					*targets.textureForWidescreen = true;
				else if (equalNoCase(value, L"No"))
					*targets.textureForWidescreen = false;
			}
		} else if (equalNoCase(name, L"Brightness")) {
			if (targets.brightness) {
				if (percent(valueText, number))
					*targets.brightness = number;
				else if (wxLog::loglevel > 0)
					wxLog::logexpanded(L"Invalid value for parameter Brightness in config file.");
			}
		} else if (equalNoCase(name, L"LogLevel")) {
			if (targets.logLevel) {
				if (equalNoCase(value, L"Error"))
					*targets.logLevel = eLogLevel::kError;
				else if (equalNoCase(value, L"Warning"))
					*targets.logLevel = eLogLevel::kWarning;
				else if (equalNoCase(value, L"Info"))
					*targets.logLevel = eLogLevel::kInfo;
				else if (equalNoCase(value, L"Max"))
					*targets.logLevel = eLogLevel::kMax;
				else if (wxLog::loglevel > 0)
					wxLog::logexpanded(L"Invalid value for parameter LogLevel in config file.");
			}
		} else if (equalNoCase(name, L"LockCursor")) {
			if (targets.lockCursor) {
				if (equalNoCase(value, L"Enabled"))
					*targets.lockCursor = true;
				else if (equalNoCase(value, L"Disabled"))
					*targets.lockCursor = false;
			}
		} else if (equalNoCase(name, L"Device")) {
			if (targets.device) {
				if (equalNoCase(value, L"DX9"))
					*targets.device = eGraphicsInterface::kDirectX9;
				else if (equalNoCase(value, L"DX11"))
					*targets.device = eGraphicsInterface::kDirectX11;
				else if (equalNoCase(value, L"OGL"))
					*targets.device = eGraphicsInterface::kOpenGL;
			}
		} else if (equalNoCase(name, L"Password")) {
			if (targets.password)
				*targets.password = valueText;
		} else {
			// (a name that is not known is a line without a place for it: the original counts it like a line
			// without `=`)
			lineWithoutEquals = true;
		}
	}

	return !lineWithoutEquals;
}
