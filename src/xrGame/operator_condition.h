////////////////////////////////////////////////////////////////////////////
//	Module 		: operator_condition.h
//	Created 	: 24.02.2004
//  Modified 	: 24.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Operator condition
////////////////////////////////////////////////////////////////////////////

#pragma once

// Template parameters renamed from `_condition_type`/`_value_type` to
// `_TConditionType`/`_TValueType` - the class used to `typedef
// _condition_type _condition_type;` (and same for _value_type), the same
// self-shadowing-typedef bug class as graph_vertex.h/intrusive_ptr.h (see
// notes section 30b): the typedef name is identical to the template
// parameter it shadows, which GCC's -Wtemplate-body flags and then treats
// the whole template as erroneous on first real instantiation (reached here
// via CConditionState<COperatorConditionAbstract<...>>::COperatorCondition::
// _condition_type in condition_state_inline.h). Fixed the same way as
// before: rename the template *parameter*, keep the exposed nested-type
// name unchanged since external code (condition_state.h/_inline.h) looks it
// up unqualified as `_condition_type`/`_value_type`.
template <
	typename _TConditionType,
	typename _TValueType
>
class COperatorConditionAbstract
{
public:
	typedef _TConditionType _condition_type;
	typedef _TValueType _value_type;

protected:
	typedef COperatorConditionAbstract<_TConditionType, _TValueType> COperatorCondition;

protected:
	_TConditionType m_condition;
	u32 m_hash;
	_TValueType m_value;

public:
	IC COperatorConditionAbstract(const _TConditionType condition, const _TValueType value);
	IC const _TConditionType& condition() const;
	IC const _TValueType& value() const;
	IC const u32& hash_value() const;
	IC bool operator<(const COperatorCondition& condition) const;
	IC bool operator==(const COperatorCondition& condition) const;
};

#include "operator_condition_inline.h"
