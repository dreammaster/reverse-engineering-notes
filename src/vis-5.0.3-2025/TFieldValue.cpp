#include "TFieldValue.h"

#include <clocale>
#include <cstdio>

// Confirmed (asm lines 528225-528243)
TFieldValue::TFieldValue(bool value) : _bool(value), _kind(kBool) {
}

// Confirmed (asm lines 528244-528262)
TFieldValue::TFieldValue(long value) : _int(value), _kind(kInt) {
}

// Confirmed (asm lines 528263-528281)
TFieldValue::TFieldValue(float value) : _float(value), _kind(kFloat) {
}

// Confirmed (asm lines 528282-528328)
TFieldValue::TFieldValue(const wxString &value) : _string(value), _kind(kString) {
}

// Confirmed (asm lines 528329-528388)
TFieldValue::TFieldValue(const wxFileName &value) : _kind(kPath) {
	if (value.IsOk())
		_path = value;
}

// Confirmed (asm lines 528389-528587)
TFieldValue::TFieldValue(const wxFileName &value, const wxFileName &base) : _path(value), _kind(kPath) {
	if (base.IsOk() && value.IsOk()) {
		wxFileName baseDir(base.GetFullPath().ToStdWstring());
		baseDir.NormalizePath();
		_path.MakeRelativeTo(baseDir);
	}
}

// Confirmed (asm lines 528588-529249)
wxString TFieldValue::ToString() const {
	wxString result;

	switch (_kind) {
	case kInt: {
		wchar_t buffer[32];
		swprintf(buffer, 32, L"%ld", _int);
		return wxString(buffer);
	}
	case kBool:
		return wxString(_bool ? L"T" : L"F");
	case kFloat: {
		setlocale(LC_NUMERIC, "C");
		wchar_t buffer[64];
		swprintf(buffer, 64, L"%f", (double)_float);
		return wxString(buffer);
	}
	case kString:
		result = _string;
		break;
	case kPath:
		result = _path.GetFullPath();
		break;
	default:
		return result;
	}

	// The replacement order matters: '&' first, so the entities added by the
	// later passes aren't escaped again.
	result.Replace(L"&", L"&amp;", true);
	result.Replace(L"<", L"&lt;", true);
	result.Replace(L">", L"&gt;", true);
	result.Replace(L"\"", L"&quot;", true);
	result.Replace(L"'", L"&apos;", true);
	return result;
}
