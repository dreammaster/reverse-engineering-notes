#include "datastruct/link.h"

#include "baselib/xmlWriter.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 635588-635623)
TLink::TLink(bool any) : _id(-1, -1), _field(-1), _flags(any ? kAny : 0) {
}

// Confirmed (asm lines 635624-635659)
TLink::TLink(const TId &id, int field, bool parent) : _id(id), _field((short)field), _flags(parent ? kParent : 0) {
}

// Confirmed (asm lines 635675-635697)
TLink::TLink() : _id(-1, -1), _field(-1), _flags(0) {
}

// Confirmed (asm lines 635756-635862). Two "any" links are equal to each other
// and to nothing else; two links to nothing are equal whatever their fields.
bool TLink::operator==(const TLink &other) const {
	if (IsAnyLink() || other.IsAnyLink())
		return IsAnyLink() == other.IsAnyLink();

	if (_id.getId() == -1 && _id.getTable() == 0xFF && other._id.getId() == -1 && other._id.getTable() == 0xFF)
		return true;

	return _id == other._id && _field == other._field && IsParentLink() == other.IsParentLink();
}

// Confirmed (asm lines 635863-635893)
void TLink::Clear() {
	_id = TId(-1, -1);
	_field = -1;
	_flags = 0;
}

// Confirmed (asm lines 635961-635990)
void TLink::Serialize(int id, int table, int field, bool parent, bool any) {
	_id = TId(id, table);
	_flags = parent ? kParent : 0;
	if (any)
		_flags |= kAny;
	_field = (short)field;
}

// Confirmed (asm lines 635991-636056)
void TLink::Serialize(TProjectFileWriter &writer, int name) {
	writer.StartTag(name);
	writer.AddAttribute(kParentLink, IsParentLink());
	writer.AddAttribute(kId, _id.getId());
	writer.AddAttribute(kTableId, (int)(signed char)_id.getTable());
	writer.AddAttribute(kLinkAny, IsAnyLink());
	writer.FinishAttributes(false);
}

// Confirmed (asm lines 636057-636138)
bool TLink::Serialize(TProjectFileWriter &writer, int name, std::vector<TLink> &links) {
	writer.StartTagWithoutAttributes(name);
	for (TLink &link : links)
		link.Serialize(writer, kLink);
	writer.FinishTag(name);
	return true;
}
