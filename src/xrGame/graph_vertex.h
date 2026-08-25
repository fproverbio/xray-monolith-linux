////////////////////////////////////////////////////////////////////////////
//	Module 		: CVertex.h
//	Created 	: 14.01.2004
//  Modified 	: 19.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Graph vertex base class template
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "object_broker.h"

template <
	typename _data_type,
	typename _vertex_id_type,
	typename _graph_type
>
class CVertex
{
public:
	typedef _vertex_id_type _vertex_id_type;
	typedef typename _graph_type::CEdge _edge_type;
	// KNOWN GENUINE BLOCKER (documented, not mechanically fixed this pass -
	// see playground/xray-monolith-vulkan-port-notes.md's integration-pass
	// section): this line is a real mutual-recursive-instantiation cycle
	// under GCC. CGraphAbstract::EDGES needs CVertex complete;
	// CVertex::_edge_weight_type (this line) needs _edge_type (CEdge<...>)
	// complete; CEdge's own CEdgeBase base needs CVertex::_vertex_id_type -
	// i.e. a request for a member of *this exact CVertex specialization*
	// that loops back in through a different template while this
	// specialization is still mid-instantiation. GCC treats a class
	// template as wholly "incomplete" to any external nested-name lookup
	// for its entire instantiation, even for members declared/processed
	// earlier in the same pass (tried routing through _graph_type
	// directly instead - same wall, just one hop later). The original
	// MSVC target tolerated this; standard C++/GCC does not. A real fix
	// needs restructuring this three-way CGraphAbstract/CVertex/CEdge
	// dependency (e.g. an explicit _edge_weight_type template parameter
	// on CVertex instead of deriving it via nested lookup) - out of scope
	// for a mechanical port pass; left as-is, real bug, not guessed at.
	typedef typename _edge_type::_edge_weight_type _edge_weight_type;
	typedef xr_vector<_edge_type> EDGES;
	typedef xr_vector<CVertex*> VERTICES;

private:
	_vertex_id_type m_vertex_id;
	EDGES m_edges;
	_data_type m_data;
	// this container holds vertices, which has edges to us
	// this is needed for the fast vertex removal
	VERTICES m_vertices;
	// this counter is use for fast edge count computation in graph
	size_t* m_edge_count;

public:
	IC CVertex(const _data_type& data, const _vertex_id_type& vertex_id, size_t* edge_count);
	IC ~CVertex();
	IC bool operator==(const CVertex& obj) const;
	IC void add_edge(CVertex* vertex, const _edge_weight_type& edge_weight);
	IC void remove_edge(const _vertex_id_type& vertex_id);
	IC void on_edge_addition(CVertex* vertex);
	IC void on_edge_removal(const CVertex* vertex);
	IC const _vertex_id_type& vertex_id() const;
	IC const _data_type& data() const;
	IC _data_type& data();
	IC void data(const _data_type& data);
	IC const EDGES& edges() const;
	IC const _edge_type* edge(const _vertex_id_type& vertex_id) const;
	IC _edge_type* edge(const _vertex_id_type& vertex_id);
};

#include "graph_vertex_inline.h"
