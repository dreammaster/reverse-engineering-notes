#include "datastruct/visionaire.h"

TVisObjRef TVisionaire::GetGame() const {
	return TVisObjRef();
}

void TVisionaire::GetList(int /*fieldId*/, TVList &/*outList*/, bool /*flag*/) const {
}

TVisObjRef TVisionaire::CreateActiveObject(int /*typeId*/, const TVisObjRef &/*source*/) {
	return TVisObjRef();
}

TVisObjRef TVisionaire::GetEmptyObject() const {
	return TVisObjRef();
}

TVisObjRef TVisionaire::GetAnyObject() const {
	return TVisObjRef();
}

void TVisionaire::SaveSaveGame(TProjectFileWriter &/*writer*/) {
}

void TVisionaire::ResetActiveData(eVisionaireTable /*table*/) {
}

bool TVisionaire::LoadDataGame(const wxFileName &/*file*/, const wxString &/*extra*/, TLoadingTypeEnum /*type*/,
                               bool /*flag*/, TSignalSlot */*slot*/, EventHandler */*handler*/) {
	return false;
}

bool TVisionaire::Load(const wxFileName &/*file*/, const wxString &/*extra*/, eSaveGame /*saveGame*/,
                       TLoadingTypeEnum /*type*/, int */*outFlag*/, TSignalSlot */*slot*/,
                       EventHandler */*handler*/) {
	return false;
}

bool TVisionaire::LoadSaveGame(const wxFileName &/*file*/, const wxString &/*extra*/) {
	return false;
}

TVisionaireObject *TVisionaire::GetObjectById(const TId &/*id*/) const {
	return nullptr;
}

void TVisionaire::RemoveLink(const TId &/*from*/, const TId &/*to*/, int /*field*/) {
}

bool TVisionaire::IsLinkRemovalSuppressed() const {
	return false;
}

bool TVisionaire::IsVisPlayerMode = false;

void TVisionaire::SetDirty() {
}

bool TVisionaire::HasIdMapping() const {
	return false;
}

TId TVisionaire::GetMappedId(const TId &/*id*/) const {
	return TId(-1, -1);
}

void TVisionaire::AddLink(const TId &/*from*/, const TId &/*to*/, int /*field*/, bool /*flag*/) {
}

void TVisionaire::RemoveObjectByParent(TVisionaireObject */*object*/) {
}

int TVisionaire::ActiveInstances = 0;

void TVisionaire::SetModified(bool /*modified*/) {
}

bool TVisionaire::GetTable(int /*table*/, TTable ** /*outTable*/) const {
	return false;
}

bool TVisionaire::ChangeOrder(const TVisObjRef & /*object*/, TMoveOrderEnum /*move*/) {
	return false;
}

void TVisionaire::GetObjectsLinkedTo(const TId & /*id*/, std::vector<TLinkRef> & /*out*/) const {
}

void TVisionaire::RemoveObject(TVisionaireObject * /*object*/) {
}

int TVisionaire::GetModifiedStamp() const {
	return 0;
}
