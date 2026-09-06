////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_base.h
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction base
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "space_restriction_abstract.h"

class CSpaceRestrictionBase : public CSpaceRestrictionAbstract
{
private:
	typedef CSpaceRestrictionAbstract inherited;

public:
#ifdef DEBUG
	xr_vector<u32>		m_test_storage;
	bool				m_correct;
#endif

protected:
	void process_borders();

public:
	bool inside(u32 level_vertex_id, bool partially_inside);
	bool inside(u32 level_vertex_id, bool partially_inside, float radius);
	virtual bool inside(const Fsphere& sphere) = 0;
	virtual bool shape() const = 0;
	virtual bool default_restrictor() const = 0;
	virtual Fsphere sphere() const = 0;

	// No-op for most implementations; CSpaceRestrictionComposition overrides
	// this to release its intrusive_ptr<CSpaceRestrictionBridge> references
	// to sibling entries of the same CSpaceRestrictionHolder::m_restrictions
	// map. CSpaceRestrictionHolder::clear() must call this on every bridge's
	// implementation before it starts deleting bridges: bridges use a
	// Deferred intrusive_ptr policy (dropping to zero refs doesn't free
	// them), but clear() itself frees every bridge unconditionally and in
	// map key order - if a composition's cross-reference to a sibling bridge
	// is still alive when that sibling gets deleted (because the sibling's
	// map slot happens to sort first), the composition's destructor later
	// runs an intrusive_ptr::dec() against already-freed memory. Releasing
	// every composition's references first (while all siblings are still
	// alive) makes the later straight delete-in-map-order pass safe.
	virtual void release_dependencies() {}

public:
#ifdef DEBUG
	IC		bool		correct				() const;
#endif
};

#include "space_restriction_base_inline.h"
