#pragma once

template <typename T>
struct CWrapperBase : public T, public ::luabind::wrap_base
{
	typedef T inherited;
	typedef CWrapperBase<T> self_type;

	virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action)
	{
		return luabind::call_member<bool>(this, "OnKeyboard", dik, keyboard_action);
	}

	static bool OnKeyboard_static(inherited* ptr, int dik, EUIMessages keyboard_action)
	{
		return ptr->self_type::inherited::OnKeyboardAction(dik, keyboard_action);
	}

	virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action)
	{
		return luabind::call_member<bool>(this, "OnMouse", x, y, mouse_action);
	}

	static bool OnMouse_static(inherited* ptr, float x, float y, EUIMessages mouse_action)
	{
		return ptr->self_type::inherited::OnMouseAction(x, y, mouse_action);
	}

	virtual void Update()
	{
		luabind::call_member<void>(this, "Update");
	}

	static void Update_static(inherited* ptr)
	{
		ptr->self_type::inherited::Update();
	}

	virtual bool Dispatch(int cmd, int param)
	{
		return luabind::call_member<bool>(this, "Dispatch", cmd, param);
	}

	static bool Dispatch_static(inherited* ptr, int cmd, int param)
	{
		return ptr->self_type::inherited::Dispatch(cmd, param);
	}
};

typedef CWrapperBase<CUIDialogWndEx> WrapType;
typedef CUIDialogWndEx BaseType;

// The 3rd class_<> template argument is HolderType (defaults to
// null_type), NOT the Lua-override WrapperType - that's the 4th
// argument (see UIListBox_script.cpp's identical fix for the full
// explanation). WrapType was landing in the HolderType slot here.
typedef luabind::class_<CUIDialogWndEx, luabind::bases<CUIDialogWnd, DLL_Pure>, luabind::null_type, WrapType> export_class;
