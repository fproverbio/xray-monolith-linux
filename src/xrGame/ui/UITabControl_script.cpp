#include "pch_script.h"
#include "UITabControl.h"
#include "UITabButton.h"

using namespace luabind;
// luabind::policy::adopt is in a nested namespace - "using namespace luabind;"
// alone does not pull in unqualified adopt<N>() below (same-shape gap as
// the get_globals->globals/.type() API-drift fixes elsewhere in this port).
using namespace luabind::policy;

#pragma optimize("s",on)
void CUITabControl::script_register(lua_State* L)
{
	module(L)
	[
		class_<CUITabControl, CUIWindow>("CUITabControl")
		.def(constructor<>())
		.def("AddItem", (bool (CUITabControl::*)(CUITabButton*))(&CUITabControl::AddItem), adopt<2>())
		.def("AddItem", (bool (CUITabControl::*)(LPCSTR, LPCSTR, Fvector2, Fvector2))&CUITabControl::AddItem)
		.def("RemoveAll", &CUITabControl::RemoveAll)
		.def("AddTab", &CUITabControl::AddTab)
		.def("SetTabIcon", &CUITabControl::SetTabIcon)
		.def("RecalcScroll", &CUITabControl::RecalcScroll)
		.def("GetActiveId", &CUITabControl::GetActiveId_script)
		.def("GetTabsCount", &CUITabControl::GetTabsCount)
		.def("SetActiveTab", &CUITabControl::SetActiveTab_script)
		.def("GetButtonById", &CUITabControl::GetButtonById_script)
		.def("GetEnabled", &CUITabControl::GetAcceleratorsMode)
		.def("SetEnabled", &CUITabControl::SetAcceleratorsMode),

		class_<CUITabButton, CUIButton>("CUITabButton")
		.def(constructor<>())
	];
}
