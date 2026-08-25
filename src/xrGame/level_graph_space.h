////////////////////////////////////////////////////////////////////////////
//	Module 		: level_graph_space.h
//	Created 	: 02.10.2001
//  Modified 	: 08.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Level graph space
////////////////////////////////////////////////////////////////////////////

#pragma once

// Forward declaration needed before the `friend class ::CLevelGraph;` below
// (see its own comment) - a qualified elaborated-type-specifier can only
// befriend an *already-declared* entity, it can't implicitly declare one
// the way an unqualified `friend class X;` can. Whichever TU happens to
// #include this header pulls in level_graph.h (this header's only real
// includer), which itself #includes this file *before* its own `class
// CLevelGraph { ... }` definition further down - so without this forward
// declaration, the friend statement below depends on some unrelated header
// elsewhere in the TU's chain (ai_space.h/patrol_point.h/etc., which each
// carry their own leaf `class CLevelGraph;` forward decl for unrelated
// reasons) having already run first purely by include-order coincidence.
// Real bug, not cosmetic: level_graph.cpp itself has no such earlier
// includer, and failed with "'CLevelGraph' in namespace '::' does not name
// a type" until this was added.
class CLevelGraph;

namespace LevelGraph
{
	class CHeader : private hdrNODES
	{
	private:
		friend class CRenumbererConverter;

	public:
		ICF u32 version() const;
		ICF u32 vertex_count() const;
		ICF float cell_size() const;
		ICF float factor_y() const;
		ICF const Fbox& box() const;
		ICF const xrGUID& guid() const;
	};

	typedef NodePosition CPosition;

	class CVertex : private NodeCompressed
	{
	private:
		friend class CRenumbererConverter;

	public:
		ICF u32 link(int i) const;
		ICF u16 high_cover(u8 index) const;
		ICF u16 low_cover(u8 index) const;
		ICF u16 plane() const;
		ICF const CPosition& position() const;
		ICF bool operator<(const LevelGraph::CVertex& vertex) const;
		ICF bool operator>(const LevelGraph::CVertex& vertex) const;
		ICF bool operator==(const LevelGraph::CVertex& vertex) const;
		// Qualified (::CLevelGraph) - an unqualified `friend class CLevelGraph;`
		// here, inside `namespace LevelGraph`, does NOT bind to the real
		// global ::CLevelGraph (defined later in level_graph.h): standard
		// friend-name lookup only searches the innermost enclosing scope
		// (here, namespace LevelGraph) for a prior matching declaration, and
		// since none exists there, it silently declares a brand new, always-
		// incomplete LevelGraph::CLevelGraph instead - a distinct, phantom
		// class that never grants the real CLevelGraph any access. MSVC is
		// permissive here and resolves it to the real global class anyway;
		// GCC correctly enforces the narrower standard rule, surfacing as
		// "NodeCompressed::p is inaccessible" from CLevelGraph::contour()
		// (level_graph_vertex_inline.h) - confirmed via a minimal repro
		// before landing this fix, not just theory.
		friend class ::CLevelGraph;
	};

	struct SSegment
	{
		Fvector v1;
		Fvector v2;
	};

	struct SContour : public SSegment
	{
		Fvector v3;
		Fvector v4;
	};
};
