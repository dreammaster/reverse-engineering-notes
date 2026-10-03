// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/datastruct/datagrp.cpp - see manifest/source_layout.tsv.
//
// TDataGroup is one game-data record: a block of typed field storage laid out
// by its TTypeGroup (see datastruct/data.h for the per-kind handling) plus the
// links, event handlers and flags around it. Only the owner pointer (+0x20 of
// the original, read by TData::DeleteDataInstance) is modeled so far; the
// other 40 methods are not reconstructed yet.
#pragma once

class TVisionaireObject;

class TDataGroup {
public:
	TVisionaireObject *GetOwner() const {
		return _owner;
	}

private:
	TVisionaireObject *_owner = nullptr;
};
