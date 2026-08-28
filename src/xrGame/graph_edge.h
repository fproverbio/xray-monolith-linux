////////////////////////////////////////////////////////////////////////////
//	Module 		: CEdge.h
//	Created 	: 14.01.2004
//  Modified 	: 19.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Graph edge class template
////////////////////////////////////////////////////////////////////////////

#pragma once

// Template parameters renamed from `_edge_weight_type`/`_vertex_type` to
// `_TEdgeWeightType`/`_TVertexType` - the class used to
// `typedef _edge_weight_type _edge_weight_type;` (and same for
// _vertex_type), the same self-shadowing-typedef bug class as
// operator_condition.h/a_star.h/vertex_manager_fixed.h. GCC's
// -Wtemplate-body flags this and poisons the whole class template on
// instantiation ("instantiating erroneous template"), even though nothing
// actually goes wrong at real use. Exposed nested-typedef names kept
// unchanged since external code (graph_vertex.h etc.) looks them up
// unqualified as `_edge_type::_edge_weight_type`.
template <typename _TEdgeWeightType, typename _TVertexType>
class CEdgeBase
{
public:
	typedef _TEdgeWeightType _edge_weight_type;
	typedef _TVertexType _vertex_type;
	typedef typename _TVertexType::_vertex_id_type _vertex_id_type;

private:
	_TEdgeWeightType m_weight;
	_TVertexType* m_vertex;

public:
	IC CEdgeBase(const _TEdgeWeightType& weight, _TVertexType* vertex);
	IC const _TEdgeWeightType& weight() const;
	IC _TVertexType* vertex() const;
	IC const _vertex_id_type& vertex_id() const;
};

template <typename _edge_weight_type, typename _vertex_type, typename _edge_data_type>
class CEdge :
	public CEdgeBase<_edge_weight_type,_vertex_type>
{
private:
	typedef CEdgeBase<_edge_weight_type, _vertex_type> inherited;
	// Same missing-typename-import gap as the CEdge<...,xr_empty>
	// partial specialization below (_vertex_id_type is a member of the
	// dependent base CEdgeBase<...>, unqualified lookup in a template
	// doesn't search dependent bases) - GCC's -Wtemplate-body structurally
	// checks this primary template's body even though nothing in this
	// port currently instantiates it (every real CGraphAbstract use goes
	// through the xr_empty specialization), so it still needs fixing to
	// keep the whole header well-formed.
	using _vertex_id_type = typename inherited::_vertex_id_type;

private:
	_edge_data_type m_data;

public:
	IC CEdge(const _edge_weight_type& weight, _vertex_type* vertex);
	IC bool operator==(const _vertex_id_type& vertex_id) const;
	IC bool operator==(const CEdge& obj) const;
	IC const _edge_data_type& data() const;
	IC _edge_data_type& data();
};

template <typename _edge_weight_type, typename _vertex_type>
class CEdge<_edge_weight_type, _vertex_type, xr_empty> : 
	public CEdgeBase<_edge_weight_type,_vertex_type> {
private:
	typedef CEdgeBase<_edge_weight_type, _vertex_type> inherited;
	using _vertex_id_type = typename inherited::_vertex_id_type;

public:
	IC CEdge(const _edge_weight_type& weight, _vertex_type* vertex);
	IC bool operator==(const _vertex_id_type& vertex_id) const;
	IC bool operator==(const CEdge& obj) const;
};

#include "graph_edge_inline.h"
