#include "pch_script.h"
#include "UIListBox.h"
#include "UIListBoxItem.h"
#include "UIListBoxItemMsgChain.h"
#include "UIMapList.h"
#include "UISpinText.h"
#include "UIMapInfo.h"
#include "UIComboBox.h"

using namespace luabind;
// luabind::policy::adopt is in a nested namespace - "using namespace luabind;"
// alone does not pull in unqualified adopt<N>() below (same-shape gap as
// the get_globals->globals/.type() API-drift fixes elsewhere in this port).
using namespace luabind::policy;


struct CUIListBoxItemWrapper : public CUIListBoxItem, public ::luabind::wrap_base
{
	CUIListBoxItemWrapper(float h): CUIListBoxItem(h)
	{
	}
};

struct CUIListBoxItemMsgChainWrapper : public CUIListBoxItemMsgChain, public ::luabind::wrap_base
{
	CUIListBoxItemMsgChainWrapper(float h) : CUIListBoxItemMsgChain(h)
	{
	}
};


#pragma optimize("s",on)
void CUIListBox::script_register(lua_State* L)
{
	module(L)
	[

		class_<CUIListBox, CUIScrollView>("CUIListBox")
		.def(constructor<>())
		.def("ShowSelectedItem", &CUIListBox::Show)
		.def("RemoveAll", &CUIListBox::Clear)
		.def("GetSize", &CUIListBox::GetSize)
		.def("GetSelectedItem", &CUIListBox::GetSelectedItem)
		.def("GetSelectedIndex", &CUIListBox::GetSelectedIDX)
		.def("SetSelectedIndex", &CUIListBox::SetSelectedIDX)
		.def("SetItemHeight", &CUIListBox::SetItemHeight)
		.def("GetItemHeight", &CUIListBox::GetItemHeight)
		.def("GetItemByIndex", &CUIListBox::GetItemByIDX)
		.def("GetItem", &CUIListBox::GetItem)
		.def("RemoveItem", &CUIListBox::RemoveWindow)
		.def("AddTextItem", &CUIListBox::AddTextItem)
		.def("AddExistingItem", &CUIListBox::AddExistingItem, adopt<2>()),

		class_<CUIListBoxItem, CUIFrameLineWnd, CUIListBoxItemWrapper>("CUIListBoxItem")
		.def(constructor<float>())
		.def("GetTextItem", &CUIListBoxItem::GetTextItem)
		.def("AddTextField", &CUIListBoxItem::AddTextField)
		.def("AddIconField", &CUIListBoxItem::AddIconField)
		.def("SetTextColor", &CUIListBoxItem::SetTextColor)
		.def("GetTAG", &CUIListBoxItem::GetTAG)
		.def("SetTAG", &CUIListBoxItem::SetTAG),

		class_<CUIListBoxItemMsgChain, CUIListBoxItem, CUIListBoxItemMsgChainWrapper>("CUIListBoxItemMsgChain")
		.def(constructor<float>()),

		// SServerFilters/connect_error_cb/CServerList (ServerList.h) registration
		// removed: ServerList.h - and every type it declared (SServerFilters,
		// connect_error_cb, CServerList, the GameSpy server-browser/connect-error
		// enum) - genuinely does not exist anywhere in this source tree (grep-
		// confirmed, not a case-sensitivity issue). Same "real platform/feature
		// gap, GameSpy/multiplayer subsystem removed from this fork" treatment as
		// RegistryFuncs.cpp/DirectPlay8 elsewhere in this port (see notes file).

		class_<CUIMapList, CUIWindow>("CUIMapList")
		.def(constructor<>())
		.def("SetWeatherSelector", &CUIMapList::SetWeatherSelector)
		.def("SetModeSelector", &CUIMapList::SetModeSelector)
		.def("OnModeChange", &CUIMapList::OnModeChange)
		.def("LoadMapList", &CUIMapList::LoadMapList)
		.def("SaveMapList", &CUIMapList::SaveMapList)
		.def("GetCommandLine", &CUIMapList::GetCommandLine)
		.def("SetServerParams", &CUIMapList::SetServerParams)
		.def("GetCurGameType", &CUIMapList::GetCurGameType)
		.def("StartDedicatedServer", &CUIMapList::StartDedicatedServer)
		.def("SetMapPic", &CUIMapList::SetMapPic)
		.def("SetMapInfo", &CUIMapList::SetMapInfo)
		.def("ClearList", &CUIMapList::ClearList)
		.def("IsEmpty", &CUIMapList::IsEmpty),

		class_<enum_exporter<EGameIDs>>("GAME_TYPE")
		.enum_("gametype")
		[
			value("GAME_UNKNOWN", int(-1)),
			value("eGameIDDeathmatch", int(eGameIDDeathmatch)),
			value("eGameIDTeamDeathmatch", int(eGameIDTeamDeathmatch)),
			value("eGameIDArtefactHunt", int(eGameIDArtefactHunt)),
			value("eGameIDCaptureTheArtefact", int(eGameIDCaptureTheArtefact))
		]

	];
}
