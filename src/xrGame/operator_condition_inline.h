////////////////////////////////////////////////////////////////////////////
//	Module 		: operator_condition_inline.h
//	Created 	: 24.02.2004
//  Modified 	: 24.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Operator condition inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "random32.h"

#define TEMPLATE_SPECIALIZATION template<\
	typename _TConditionType,\
	typename _TValueType\
>

#define CAbstractOperatorCondition COperatorConditionAbstract<_TConditionType,_TValueType>

TEMPLATE_SPECIALIZATION
IC CAbstractOperatorCondition::COperatorConditionAbstract(const _TConditionType condition, const _TValueType value) :
	m_condition(condition),
	m_value(value)
{
	u32 seed = ::Random32.seed();
	::Random32.seed(u32(condition) + 1);
	m_hash = ::Random32.random(0xffffffff);
	::Random32.seed(m_hash + u32(value));
	m_hash ^= ::Random32.random(0xffffffff);
	::Random32.seed(seed);
}

TEMPLATE_SPECIALIZATION
IC const _TConditionType&CAbstractOperatorCondition::condition() const
{
	return (m_condition);
}

TEMPLATE_SPECIALIZATION
IC const _TValueType&CAbstractOperatorCondition::value() const
{
	return (m_value);
}

TEMPLATE_SPECIALIZATION
IC const u32&CAbstractOperatorCondition::hash_value() const
{
	return (m_hash);
}

TEMPLATE_SPECIALIZATION
IC bool CAbstractOperatorCondition::operator<(const COperatorCondition& _condition) const
{
	if (condition() < _condition.condition())
		return (true);
	if (condition() > _condition.condition())
		return (false);
	if (value() < _condition.value())
		return (true);
	return (false);
}

TEMPLATE_SPECIALIZATION
IC bool CAbstractOperatorCondition::operator==(const COperatorCondition& _condition) const
{
	if ((condition() == _condition.condition()) && (value() == _condition.value()))
		return (true);
	return (false);
}

#undef TEMPLATE_SPECIALIZATION
#undef CAbstractOperatorCondition
