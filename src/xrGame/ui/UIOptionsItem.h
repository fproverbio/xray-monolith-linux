#pragma once
#include "UIOptionsManager.h"

class CUIOptionsItem
{
public:
	enum ESystemDepends { sdNothing, sdVidRestart, sdSndRestart, sdSystemRestart, sdApplyOnChange };

public:
	CUIOptionsItem();
	virtual ~CUIOptionsItem();
	virtual void AssignProps(const shared_str& entry, const shared_str& group);
	void SetSystemDepends(ESystemDepends val) { m_dep = val; }

	static CUIOptionsManager* GetOptionsManager() { return &m_optionsManager; }

	virtual void OnMessage(LPCSTR message);

	// Pure virtual functions with an (empty) body - a real MSVC extension
	// (combining a pure-specifier `= 0` with a function-body in the same
	// declaration), illegal in standard C++: the pure-specifier and the
	// body must be split into a bare `= 0;` declaration here plus an
	// out-of-class definition below the class (same fix already applied
	// to `virtual ~X() = 0 { }` destructors elsewhere in this port - see
	// notes section 26b; this is the same bug class on ordinary virtual
	// functions instead of a destructor). Kept as real, callable no-op
	// bodies (not removed) since derived overrides are free to invoke
	// e.g. `CUIOptionsItem::SetCurrentOptValue()` as a base fallback, the
	// usual reason this idiom is used at all.
	virtual void SetCurrentOptValue() = 0; // opt->current
	virtual void SaveBackUpOptValue() = 0; // current->backup
	virtual void SaveOptValue() = 0; // current->opt
	virtual void UndoOptValue() = 0; // backup->current
	virtual bool IsChangedOptValue() const = 0; // backup!=current
	void OnChangedOptValue();

protected:
	void SendMessage2Group(LPCSTR group, LPCSTR message);


	// string
	LPCSTR GetOptStringValue();
	void SaveOptStringValue(LPCSTR val);
	// integer
	void GetOptIntegerValue(int& val, int& min, int& max);
	void SaveOptIntegerValue(int val);
	// float
	void GetOptFloatValue(float& val, float& min, float& max);
	void SaveOptFloatValue(float val);
	// bool
	bool GetOptBoolValue();
	void SaveOptBoolValue(bool val);
	// token
	LPCSTR GetOptTokenValue();
	xr_token* GetOptToken();

	shared_str m_entry;
	ESystemDepends m_dep;

	static CUIOptionsManager m_optionsManager;
};

// Out-of-class bodies for the pure virtual functions declared above - see
// the comment on their declarations. Empty, matching the original combined
// `= 0 { }` bodies exactly (IsChangedOptValue's missing return was already
// the case in that form too - a pure virtual whose base body is never
// actually meant to be reached for its return value, only for its side
// effect of existing as a callable base-class fallback).
inline void CUIOptionsItem::SetCurrentOptValue() {}
inline void CUIOptionsItem::SaveBackUpOptValue() {}
inline bool CUIOptionsItem::IsChangedOptValue() const { return false; }
