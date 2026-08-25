////////////////////////////////////////////////////////////////////////////
//	Module 		: graph_edge_inline.h
//	Created 	: 14.01.2004
//  Modified 	: 19.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Graph edge class template inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _edge_weight_type,\
	typename _vertex_type\
>

#define CSGraphEdge CEdgeBase<_edge_weight_type,_vertex_type>

TEMPLATE_SPECIALIZATION
IC CSGraphEdge::CEdgeBase(const _edge_weight_type& weight, _vertex_type* vertex)
{
	m_weight = weight;
	VERIFY(vertex);
	m_vertex = vertex;
}

// Return types use the bare template parameter names (_edge_weight_type/
// _vertex_type) directly, matching the class declaration in graph_edge.h
// exactly, rather than going through the self-referential member typedef
// (`typename CSGraphEdge::_edge_weight_type` etc.) - GCC's -Wtemplate-body
// early structural check can't resolve a self-referential
// `typedef _edge_weight_type _edge_weight_type;` through an out-of-line
// qualified lookup (a real GCC limitation, not a language rule violation -
// the pattern works fine in-class and at real instantiation), and
// poisons the whole class template as a result.
TEMPLATE_SPECIALIZATION
IC const _edge_weight_type&CSGraphEdge::weight() const
{
	return (m_weight);
}

TEMPLATE_SPECIALIZATION
IC _vertex_type*CSGraphEdge::vertex() const
{
	return (m_vertex);
}

TEMPLATE_SPECIALIZATION
IC const typename CSGraphEdge::_vertex_id_type&CSGraphEdge::vertex_id() const
{
	return (this->vertex()->vertex_id());
}

#undef TEMPLATE_SPECIALIZATION
#undef CSGraphEdge

////////////////////////////////////////////////////////////////////////////
// class CEdge
////////////////////////////////////////////////////////////////////////////

#define TEMPLATE_SPECIALIZATION template <\
	typename _edge_weight_type,\
	typename _vertex_type,\
	typename _edge_data_type\
>

#define CSGraphEdge CEdge<_edge_weight_type, _vertex_type, _edge_data_type>

TEMPLATE_SPECIALIZATION
IC CSGraphEdge::CEdge(const _edge_weight_type& weight, _vertex_type* vertex) :
	inherited(weight, vertex)
{
}

// _vertex_id_type here is a member of the dependent base CEdgeBase<...>
// (or the class's own `using` alias to it) - out-of-line member-function
// parameter types don't get the same "current instantiation" leeway as
// in-class-body uses, so it needs the fully qualified dependent name
// (same pattern as every other dependent-base-member fix this session).
TEMPLATE_SPECIALIZATION
IC bool CSGraphEdge::operator==(const typename CEdgeBase<_edge_weight_type, _vertex_type>::_vertex_id_type& vertex_id) const
{
	return (this->vertex()->vertex_id() == vertex_id);
}

TEMPLATE_SPECIALIZATION
IC bool CSGraphEdge::operator==(const CEdge& obj) const
{
	if (this->weight() != obj.weight())
		return (false);

	return (this->vertex()->vertex_id() == obj.vertex()->vertex_id());
}

TEMPLATE_SPECIALIZATION
IC const _edge_data_type&CSGraphEdge::data() const
{
	return (m_data);
}

TEMPLATE_SPECIALIZATION
IC _edge_data_type&CSGraphEdge::data()
{
	return (m_data);
}

#undef TEMPLATE_SPECIALIZATION
#undef CSGraphEdge

////////////////////////////////////////////////////////////////////////////
// class CEdge<..., xr_empty>
////////////////////////////////////////////////////////////////////////////

#define TEMPLATE_SPECIALIZATION template <\
	typename _edge_weight_type,\
	typename _vertex_type\
>

#define CSGraphEdge CEdge<_edge_weight_type, _vertex_type, xr_empty>

TEMPLATE_SPECIALIZATION
IC CSGraphEdge::CEdge(const _edge_weight_type& weight, _vertex_type* vertex) :
	inherited(weight, vertex)
{
}

// _vertex_id_type here is a member of the dependent base CEdgeBase<...>
// (or the class's own `using` alias to it) - out-of-line member-function
// parameter types don't get the same "current instantiation" leeway as
// in-class-body uses, so it needs the fully qualified dependent name
// (same pattern as every other dependent-base-member fix this session).
TEMPLATE_SPECIALIZATION
IC bool CSGraphEdge::operator==(const typename CEdgeBase<_edge_weight_type, _vertex_type>::_vertex_id_type& vertex_id) const
{
	return (this->vertex()->vertex_id() == vertex_id);
}

TEMPLATE_SPECIALIZATION
IC bool CSGraphEdge::operator==(const CEdge& obj) const
{
	if (this->weight() != obj.weight())
		return (false);

	return (this->vertex()->vertex_id() == obj.vertex()->vertex_id());
}

#undef TEMPLATE_SPECIALIZATION
#undef CSGraphEdge
