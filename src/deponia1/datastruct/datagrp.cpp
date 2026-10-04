#include "datastruct/datagrp.h"

#include <algorithm>

#include "Diagnostics.h"
#include "TXMLNames.h"
#include "baselib/xmlWriter.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/datastruct/datagrp.cpp";

static bool isEmptyId(const TId &id) {
	return id.getId() == -1 && id.getTable() == 0xFF;
}

// Confirmed (asm lines 603093-603265)
TDataGroup::TDataGroup(const TTypeGroup *typeGroup, TVisionaireObject *owner)
	: _data(nullptr), _tempData(nullptr), _owner(owner), _typeGroup(typeGroup), _lastModified(-1), _flags(0) {
	if (!TVisionaire::IsVisPlayerMode)
		_name = TVisionaireObject::GetNameNewObject();

	if (_typeGroup->GetSizeData() > 0)
		_data = new char[_typeGroup->GetSizeData()];
	if (_typeGroup->GetSizeTempData() > 0)
		_tempData = new char[_typeGroup->GetSizeTempData()];

	for (const TTypeData *type : _typeGroup->GetTypes())
		TData::CreateDataInstance(type->GetType(), GetData(*type));
}

// Confirmed (asm lines 602944-603092)
TDataGroup::~TDataGroup() {
	for (const TTypeData *type : _typeGroup->GetTypes()) {
		void *value = GetData(*type);
		if (value)
			TData::DeleteDataInstance(type->GetType(), value, this);
	}

	delete[] _data;
	delete[] _tempData;
	for (TEventHandlerAndField *handler : _handlers)
		delete handler;
}

TVisionaire *TDataGroup::visionaire() const {
	return _owner->GetVisionaire();
}

void TDataGroup::markModified() {
	visionaire()->SetDirty();
	_flags |= kDirty;
}

// Confirmed (asm lines 600389-600515)
void TDataGroup::CreateTempData() {
	x_assert(_tempData == nullptr, "m_pTempData == NULL", kSourceFile, 0x4C);

	_tempData = nullptr;
	if (_typeGroup->GetSizeTempData() <= 0)
		return;

	_tempData = new char[_typeGroup->GetSizeTempData()];
	for (const TTypeData *type : _typeGroup->GetTypes()) {
		if (type->IsTempType())
			TData::CreateDataInstance(type->GetType(), GetData(*type));
	}
}

// Confirmed (asm lines 600516-600633)
void TDataGroup::RemoveTempData() {
	if (!_tempData)
		return;

	for (const TTypeData *type : _typeGroup->GetTypes()) {
		if (type->IsTempType())
			TData::DeleteDataInstance(type->GetType(), GetData(*type), this);
	}

	delete[] _tempData;
	_tempData = nullptr;
}

// Confirmed (asm lines 600668-600770)
void TDataGroup::NotifyEvent(int field, TEventEnum event, TId *linked) {
	TVisionaireObject *object = nullptr;
	if (linked && !isEmptyId(*linked))
		object = visionaire()->GetObjectById(*linked);

	// A handler may unregister itself while it's being called.
	std::vector<TEventHandlerAndField *> handlers(_handlers);
	for (TEventHandlerAndField *handler : handlers) {
		if (handler->event == event)
			handler->handler->OnEvent(event, field, object);
	}
}

// Confirmed (asm lines 603266-603377): registering a handler again replaces its
// earlier registration.
void TDataGroup::RegisterEventHandler(TEventHandlerInterface *handler, TEventEnum event) {
	UnRegisterEventHandler(handler);

	TEventHandlerAndField *registration = new TEventHandlerAndField;
	registration->handler = handler;
	registration->event = event;
	_handlers.push_back(registration);
}

// Confirmed (asm lines 602869-602943)
void TDataGroup::UnRegisterEventHandler(TEventHandlerInterface *handler) {
	for (auto it = _handlers.begin(); it != _handlers.end(); ++it) {
		if ((*it)->handler == handler) {
			delete *it;
			_handlers.erase(it);
			return;
		}
	}
}

// Confirmed (asm lines 601431-601502)
void *TDataGroup::GetData(const TTypeData &type) const {
	if (!type.IsTempType() && _data && type.GetOffset() != -1)
		return _data + type.GetOffset();
	if (type.IsTempType() && _tempData && type.GetOffset() != -1)
		return _tempData + type.GetOffset();
	return nullptr;
}

// Confirmed (asm lines 601503-601759)
void *TDataGroup::GetValue(int field, eTypeData type) const {
	const TTypeData &typeData = _typeGroup->GetTypeData(field);

	if (typeData.GetType() == type && typeData.GetOffset() != -1) {
		void *value = GetData(typeData);
		if (value)
			return value;

		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"TDataGroup::GetValue: empty field");
		return nullptr;
	}

	if (wxLog::loglevel > 0)
		wxLog::logexpanded(L"TDataGroup::GetValue: Cannot find field in object: %ls", TXMLNames::GetString(field).c_str());
	return nullptr;
}

// Confirmed (asm lines 601106-601430)
bool TDataGroup::SetValue(int field, const void *value, eTypeData type, TSendEventEnum event) {
	const TTypeData &typeData = _typeGroup->GetTypeData(field);

	if (typeData.GetType() != type) {
		bool stringOrPath = (type == eTypeData::kPath && typeData.GetType() == eTypeData::kString) ||
		                    (type == eTypeData::kString && typeData.GetType() == eTypeData::kPath);
		if (!stringOrPath)
			return false;
	}

	// ValueInt (0x148) and ValueFloat (0x325) hold the same number twice.
	if (field == kValueInt) {
		float mirrored = (float)*static_cast<const int *>(value);
		TData::Assign(eTypeData::kFloat, GetData(_typeGroup->GetTypeData(kValueFloat)), &mirrored);
	} else if (field == kValueFloat) {
		int mirrored = (int)*static_cast<const float *>(value);
		TData::Assign(eTypeData::kInt, GetData(_typeGroup->GetTypeData(kValueInt)), &mirrored);
	}

	void *stored = GetData(typeData);
	if (event != TSendEventEnum::kForce && TData::CompareDataWithValue(type, stored, value) == 0)
		return true;

	markModified();
	TData::Assign(type, stored, value);

	if (event != TSendEventEnum::kNoEvent)
		NotifyEvent(field, TEventEnum::kChanged, nullptr);
	return true;
}

// Confirmed (asm lines 600771-601105 and 601942-601981)
bool TDataGroup::RemoveLink(int field, const TId &id, bool notify) {
	if (isEmptyId(id))
		return false;
	return removeLinkOf(field, id, notify);
}

bool TDataGroup::removeLinkOf(int field, const TId &id, bool notify) {
	TId removedParent(-1, -1);
	bool removed = false;

	eTypeData type = _typeGroup->GetType(field, true);
	if (type == eTypeData::kLink) {
		TLink &link = GetRefLink(GetData(_typeGroup->GetTypeData(field)));
		TId oldId = link.GetId();

		if (link.IsParentLink())
			removedParent = link.GetId();
		else
			visionaire()->RemoveLink(_owner->GetTId(), id, field);

		link.Clear();
		markModified();
		if (notify)
			NotifyEvent(field, TEventEnum::kChanged, &oldId);
		removed = true;
	} else if (type == eTypeData::kLinkList) {
		std::vector<TLink> &links = GetRefLinks(GetData(_typeGroup->GetTypeData(field)));

		for (auto it = links.begin(); it != links.end(); ++it) {
			if (!(it->GetId() == id))
				continue;

			if (it->IsParentLink())
				removedParent = it->GetId();
			else
				visionaire()->RemoveLink(_owner->GetTId(), id, field);

			links.erase(it);
			markModified();
			removed = true;
			break;
		}
	} else {
		x_assert(false, "false", kSourceFile, 0x179);
	}

	// Removing a parent link removes the child that was linked.
	if (!isEmptyId(removedParent)) {
		TVisionaireObject *object = visionaire()->GetObjectById(removedParent);
		if (object)
			visionaire()->RemoveObjectByParent(object);
	}
	return removed;
}

// Confirmed (asm lines 601760-601941). The flag isn't read. A parent link with
// an empty id can't be removed through RemoveLink(); the original would loop
// forever on it, this drops it directly.
void TDataGroup::ClearLinks(int field, bool /*notify*/) {
	std::vector<TLink> &links = GetRefLinks(GetValue(field, eTypeData::kLinkList));
	markModified();

	while (!links.empty()) {
		TLink link = links.front();

		if (link.IsParentLink()) {
			if (!isEmptyId(link.GetId())) {
				removeLinkOf(field, link.GetId(), false);
				continue;
			}
		} else if (!isEmptyId(link.GetId())) {
			visionaire()->RemoveLink(_owner->GetTId(), link.GetId(), link.GetField());
		}
		links.erase(links.begin());
	}
}

// Confirmed (asm lines 601982-602427)
void TDataGroup::ValidateAndAdaptLinks() {
	for (const TTypeData *type : _typeGroup->GetTypes()) {
		if (type->GetType() == eTypeData::kLink) {
			TLink &link = GetRefLink(GetData(*type));
			if (isEmptyId(link.GetId()))
				continue;

			TId mapped = visionaire()->GetMappedId(link.GetId());
			if (isEmptyId(mapped)) {
				if (visionaire()->GetObjectById(link.GetId()))
					continue;

				// The object it pointed to is gone.
				x_assert(!link.IsParentLink(), "!link.IsParentLink()", kSourceFile, 0x2B9);
				if (!link.IsParentLink())
					visionaire()->RemoveLink(_owner->GetTId(), link.GetId(), link.GetField());
				link.Clear();
				continue;
			}

			if (!link.IsParentLink()) {
				visionaire()->RemoveLink(_owner->GetTId(), link.GetId(), link.GetField());
				visionaire()->AddLink(_owner->GetTId(), mapped, link.GetField(), true);
			}
			link.SetId(mapped);
		} else if (type->GetType() == eTypeData::kLinkList) {
			std::vector<TLink> &links = GetRefLinks(GetData(*type));

			for (auto it = links.begin(); it != links.end();) {
				TId mapped = visionaire()->GetMappedId(it->GetId());

				if (!isEmptyId(mapped)) {
					if (!it->IsParentLink()) {
						visionaire()->RemoveLink(_owner->GetTId(), it->GetId(), it->GetField());
						visionaire()->AddLink(_owner->GetTId(), mapped, it->GetField(), true);
					}
					it->SetId(mapped);
					++it;
				} else if (visionaire()->GetObjectById(it->GetId())) {
					++it;
				} else {
					x_assert(!it->IsParentLink(), "!it->IsParentLink()", kSourceFile, 0x2E1);
					if (!it->IsParentLink())
						visionaire()->RemoveLink(_owner->GetTId(), it->GetId(), it->GetField());
					it = links.erase(it);
				}
			}
		}
	}
}

// Confirmed (asm lines 602428-602464): the id and the order go into the
// owner, the last-modified stamp and the name stay here.
void TDataGroup::Serialize(int id, int order, int lastModified, const TCharHolder &name) {
	_owner->SetId24(id);
	_owner->SetOrder24(order);
	_lastModified = lastModified & 0xFFFFFF;
	if (_lastModified & 0x800000)
		_lastModified -= 0x1000000;
	_name = name;
}

// Confirmed (asm lines 602465-602507)
bool TDataGroup::SetName(const TCharHolder &name) {
	if (!(_name != name))
		return false;

	_name = name;
	markModified();
	return true;
}

// Confirmed (asm lines 602730-602745)
TId TDataGroup::GetId() const {
	return _owner->GetTId();
}

// Confirmed (asm lines 602542-602576)
unsigned long TDataGroup::GetListSize(int field) const {
	return GetRefLinks(GetValue(field, eTypeData::kLinkList)).size();
}

// Confirmed (asm lines 602577-602701): the sizes of all the fields, plus the
// record's own 0x50 bytes and its storage block.
unsigned long TDataGroup::GetSizeMemory(bool deep) const {
	unsigned long size = 0;

	for (const TTypeData *type : _typeGroup->GetTypes())
		size += TData::GetSizeMemory(type->GetType(), GetData(*type), deep, visionaire());
	return size + 0x50 + _typeGroup->GetSizeData();
}

// Confirmed (asm lines 603378-604382)
bool TDataGroup::ForEachCall(eActivity activity, void *a, void *b) {
	const std::vector<TTypeData *> &needed = _typeGroup->GetNeededTypes();

	switch (activity) {
	case eActivity::kSerializeParams: {
		if (!a)
			return true;

		TProjectFileWriter &writer = *static_cast<TProjectFileWriter *>(a);
		for (const TTypeData *type : needed)
			TData::SerializeParam(writer, type->GetType(), type->GetDescription(), GetData(*type));
		return true;
	}
	case eActivity::kSerializeContent: {
		if (!a)
			return true;

		TProjectFileWriter &writer = *static_cast<TProjectFileWriter *>(a);
		for (const TTypeData *type : needed) {
			if (TData::SerializeContent(writer, type->GetType(), type->GetDescription(), GetData(*type)))
				continue;

			if (wxLog::loglevel > 1)
				wxLog::logexpanded(L"Problem reading from field %ls", TXMLNames::GetString(type->GetDescription()).c_str());
			x_assert(false, "false", kSourceFile, 0x1A5);
			return false;
		}
		return true;
	}
	case eActivity::kClear:
		for (const TTypeData *type : needed)
			TData::ClearDataInstance(type->GetType(), GetData(*type));
		return true;
	case eActivity::kCompare: {
		// The original reports a difference of the names when SameAs() is
		// true and compares the fields of every record pair; the evident intent
		// (differences only) is implemented.
		const TDataGroup *other = static_cast<const TDataGroup *>(a);
		TVisObjectCompareInfo *info = static_cast<TVisObjectCompareInfo *>(b);

		if (_name != other->_name) {
			TDataCompareInfo difference;
			difference.description = -1;
			difference.thisText = wxString(_name);
			difference.otherText = wxString(other->_name);
			info->differences.push_back(difference);
		}

		for (const TTypeData *type : needed) {
			void *mine = GetData(*type);
			void *theirs = other->GetData(*type);

			// Parent links aren't part of the comparison.
			if (type->GetType() == eTypeData::kLink) {
				if (GetRefLink(mine).IsParentLink() || GetRefLink(theirs).IsParentLink())
					continue;
			} else if (type->GetType() == eTypeData::kLinkList) {
				const std::vector<TLink> &links = GetRefLinks(mine);
				const std::vector<TLink> &otherLinks = GetRefLinks(theirs);
				if ((!links.empty() && links.front().IsParentLink()) ||
				    (!otherLinks.empty() && otherLinks.front().IsParentLink()))
					continue;
			}

			if (TData::Compare(type->GetType(), mine, theirs) == 0)
				continue;

			TDataCompareInfo difference;
			difference.description = type->GetDescription();
			difference.thisText = TData::ToString(type->GetType(), mine, visionaire());
			difference.otherText = TData::ToString(type->GetType(), theirs, other->visionaire());
			info->differences.push_back(difference);
		}
		return true;
	}
	}

	(void)b;
	return true;
}

// Confirmed (asm lines 604383-604507): back to the state of a new object.
void TDataGroup::Clear() {
	_name = TVisionaireObject::GetNameNewObject();
	ForEachCall(eActivity::kClear, nullptr, nullptr);
}

// Confirmed (asm lines 605492-605609): the element carries the record's
// name, id, order and last-modified stamp as attributes, then its plain
// fields as more attributes and its other fields as child elements.
bool TDataGroup::Serialize(TProjectFileWriter &writer) {
	writer.StartTag(_typeGroup->GetDescription());
	writer.AddAttributeS(kName, _name);
	writer.AddAttribute(kId, _owner->GetId24());
	writer.AddAttribute(kOrder, _owner->GetOrder24());
	writer.AddAttribute(kLastModified, _lastModified);

	if (!ForEachCall(eActivity::kSerializeParams, &writer, nullptr))
		return false;
	writer.FinishAttributes(true);
	if (!ForEachCall(eActivity::kSerializeContent, &writer, nullptr))
		return false;
	writer.FinishTag(_typeGroup->GetDescription());
	return true;
}

// Confirmed (asm lines 605610-605943)
bool TDataGroup::SetLink(int field, const TId &id, bool parent, bool notify) {
	TLink newLink(id, field, parent);

	eTypeData type = _typeGroup->GetType(field, true);
	if (type == eTypeData::kLink) {
		TLink &link = GetRefLink(GetValue(field, eTypeData::kLink));
		if (link == newLink)
			return true;

		TId oldId = link.GetId();
		if (link.IsParentLink()) {
			if (!isEmptyId(link.GetId()))
				removeLinkOf(field, link.GetId(), notify);
		} else if (!isEmptyId(link.GetId())) {
			visionaire()->RemoveLink(_owner->GetTId(), link.GetId(), link.GetField());
		}

		link = newLink;
		visionaire()->SetDirty();
		if (!isEmptyId(id) && !parent)
			visionaire()->AddLink(_owner->GetTId(), id, field, true);
		if (notify)
			NotifyEvent(field, TEventEnum::kChanged, &oldId);
		_flags |= kDirty;
		return true;
	}

	if (type == eTypeData::kLinkList) {
		if (isEmptyId(id)) {
			x_assert(false, "false", kSourceFile, 0x13E);
			return false;
		}

		std::vector<TLink> &links = GetRefLinks(GetValue(field, eTypeData::kLinkList));
		markModified();
		links.push_back(newLink);
		if (!parent)
			visionaire()->AddLink(_owner->GetTId(), id, field, true);
		return true;
	}

	x_assert(false, "false", kSourceFile, 0x141);
	return false;
}

// Confirmed (asm lines 605944-605962)
bool TDataGroup::SetLink(int field, const TId &id, bool notify) {
	return SetLink(field, id, false, notify);
}

// Confirmed (asm lines 605963-605980)
bool TDataGroup::SetParentLink(int field, const TId &id, bool notify) {
	return SetLink(field, id, true, notify);
}

// Confirmed (asm lines 605981-606021)
bool TDataGroup::ClearLink(int field, bool notify) {
	return SetLink(field, TId(-1, -1), false, notify);
}

// Confirmed (asm lines 606022-606139): clears the link, then makes it the
// "any object" link.
bool TDataGroup::SetLinkAnyObject(int field, bool notify) {
	if (_typeGroup->GetType(field, true) != eTypeData::kLink)
		return false;

	SetLink(field, TId(-1, -1), false, false);

	TLink &link = GetRefLink(GetValue(field, eTypeData::kLink));
	TLink anyLink(true);
	if (link == anyLink)
		return true;

	TId oldId = link.GetId();
	link = anyLink;
	if (notify)
		NotifyEvent(field, TEventEnum::kChanged, &oldId);
	markModified();
	return true;
}

// Confirmed (asm lines 606140-607107)
void TDataGroup::CopyContent(const TDataGroup &source, bool copyParents, std::vector<int> *skip) {
	_name = source._name;

	for (const TTypeData *type : _typeGroup->GetTypes()) {
		int field = type->GetDescription();
		if (skip && std::find(skip->begin(), skip->end(), field) != skip->end())
			continue;

		void *dest = GetData(*type);
		void *from = source.GetData(*type);

		switch (type->GetType()) {
		case eTypeData::kBool:
			GetRefBool(dest) = GetRefBool(from);
			break;
		case eTypeData::kInt:
			GetRefInt(dest) = GetRefInt(from);
			break;
		case eTypeData::kString:
			GetRefString(dest) = GetRefString(from);
			break;
		case eTypeData::kPath:
			GetRefPath(dest) = GetRefPath(from);
			break;
		case eTypeData::kFloat:
			GetRefFloat(dest) = GetRefFloat(from);
			break;
		case eTypeData::kRectList:
			GetRefVRect(dest) = GetRefVRect(from);
			break;
		case eTypeData::kSpriteList:
			GetRefVSprite(dest) = GetRefVSprite(from);
			break;
		case eTypeData::kPointList:
			GetRefVPoint(dest) = GetRefVPoint(from);
			break;
		case eTypeData::kStringList:
			GetRefVString(dest) = GetRefVString(from);
			break;
		case eTypeData::kIntList:
			GetRefVInt(dest) = GetRefVInt(from);
			break;
		case eTypeData::kPathList:
			GetRefVPath(dest) = GetRefVPath(from);
			break;
		case eTypeData::kFloatList:
			GetRefVFloat(dest) = GetRefVFloat(from);
			break;
		case eTypeData::kPoint:
			GetRefPoint(dest) = GetRefPoint(from);
			break;
		case eTypeData::kRect:
			GetRefRect(dest) = GetRefRect(from);
			break;
		case eTypeData::kSprite:
			GetRefSprite(dest) = GetRefSprite(from);
			break;
		case eTypeData::kTextList:
			GetRefVText(dest) = GetRefVText(from);
			break;
		case eTypeData::kLink: {
			TLink &destLink = GetRefLink(dest);
			TLink link = GetRefLink(from);

			if (!copyParents && link.IsParentLink())
				break;

			if (visionaire()->HasIdMapping()) {
				TId mapped = visionaire()->GetMappedId(link.GetId());
				if (!isEmptyId(mapped))
					link.SetId(mapped);
			}

			if (!isEmptyId(destLink.GetId()) && !destLink.IsParentLink())
				visionaire()->RemoveLink(_owner->GetTId(), destLink.GetId(), destLink.GetField());
			if (!isEmptyId(link.GetId()) && !link.IsParentLink())
				visionaire()->AddLink(_owner->GetTId(), link.GetId(), field, true);
			destLink = link;
			break;
		}
		case eTypeData::kLinkList: {
			std::vector<TLink> &destLinks = GetRefLinks(dest);
			std::vector<TLink> links = GetRefLinks(from);

			// The first entry says whether the list is of parent links.
			if (!copyParents && !links.empty() && links.front().IsParentLink())
				break;

			if (visionaire()->HasIdMapping()) {
				for (TLink &link : links) {
					TId mapped = visionaire()->GetMappedId(link.GetId());
					if (!isEmptyId(mapped))
						link.SetId(mapped);
				}
			}

			if (!destLinks.empty() && !destLinks.front().IsParentLink()) {
				for (const TLink &link : destLinks)
					visionaire()->RemoveLink(_owner->GetTId(), link.GetId(), link.GetField());
			}
			if (!links.empty() && !links.front().IsParentLink()) {
				for (const TLink &link : links)
					visionaire()->AddLink(_owner->GetTId(), link.GetId(), field, true);
			}
			destLinks = links;
			break;
		}
		default:
			break;
		}
	}
}

// Confirmed (asm lines 607108-607242)
bool TDataGroup::GetList(int field, TVList &out) const {
	const std::vector<TLink> &links = GetRefLinks(GetValue(field, eTypeData::kLinkList));

	out.clear();
	for (const TLink &link : links) {
		TVisionaireObject *object = visionaire()->GetObjectById(link.GetId());
		if (object)
			out.push_back(object);
	}

	std::sort(out.begin(), out.end(), [](const TVisionaireObject *a, const TVisionaireObject *b) {
		return cmpOrder(a, b);
	});
	return true;
}
