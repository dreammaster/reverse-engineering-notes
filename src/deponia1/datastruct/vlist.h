// Not yet assert-confirmed to a specific file; stays at the top level of
// datastruct/ alongside visobjref.h/visionaire.h, its evident siblings.
//
// Confirmed (Deponia_Linux.asm lines 586522-586975, 593912-594290, 594785-
// 594889): TVList is a std::vector<TVisionaireObject *> that owns one
// reference to each of its objects - adding an object takes a reference
// (asserting it isn't null), removing one releases it, and clearing or
// destroying the list releases them all (the last release deletes the object).
// It is the output type of every "list the linked objects" call
// (TVisionaire::GetList(), TVisObjRef::GetLinks(), ...). Iteration uses the
// vector's own iterators (`items` is public for that, and for the callers
// that range-for over it).
#pragma once

#include <vector>

#include "datastruct/visobjref.h"

class TVisionaireObject;

class TVList {
public:
	typedef std::vector<TVisionaireObject *>::iterator iterator;
	typedef std::vector<TVisionaireObject *>::const_iterator const_iterator;

	TVList() = default;
	TVList(const TVList &other);
	~TVList();
	TVList &operator=(const TVList &other);

	/** Releases every object and empties the list. */
	void clear();
	iterator erase(iterator position);
	void pop_back();

	TVisionaireObject *at(int index) const {
		return items.at(index);
	}
	TVisionaireObject *front() const {
		return items.front();
	}
	TVisionaireObject *back() const {
		return items.back();
	}
	iterator begin() {
		return items.begin();
	}
	iterator end() {
		return items.end();
	}
	const_iterator begin() const {
		return items.begin();
	}
	const_iterator end() const {
		return items.end();
	}
	std::vector<TVisionaireObject *>::reverse_iterator rbegin() {
		return items.rbegin();
	}
	std::vector<TVisionaireObject *>::reverse_iterator rend() {
		return items.rend();
	}
	std::size_t size() const {
		return items.size();
	}
	bool empty() const {
		return items.empty();
	}

	/** Each of these takes a reference to the object. */
	void push_back(const TVisObjRef &ref);
	void push_back(TVisionaireObject *object);
	iterator insert(iterator position, TVisionaireObject *object);
	iterator insert(iterator position, const TVisObjRef &ref);

	/** Makes this list hold references to the same objects as `other`. */
	void copy(const TVList &other);

	std::vector<TVisionaireObject *> items;
};
