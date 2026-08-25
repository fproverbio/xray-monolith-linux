#pragma once

#include "pure_relcase.h"

class ENGINE_API CObject;

namespace Feel
{
	class ENGINE_API Touch : private pure_relcase
	{
		// pure_relcase's templated constructor does static_cast<class_type*>
		// (this) to bind the relcase callback - a base-to-derived
		// conversion that needs the private-inheritance relationship to be
		// visible from pure_relcase's own code, not just Touch's. MSVC is
		// permissive about this access check across template
		// instantiation boundaries; GCC enforces it strictly (only
		// surfaces once Touch::Touch() actually instantiates the
		// constructor with class_type=Touch, same "first instantiation"
		// pattern as this port's other MSVC-permissive template bugs).
		friend class pure_relcase;
	public:
		struct DenyTouch
		{
			CObject* O;
			DWORD Expire;
		};

	protected:
		xr_vector<DenyTouch> feel_touch_disable;

	public:
		xr_vector<CObject*> feel_touch;
		xr_vector<CObject*> q_nearest;

	public:
		void __stdcall feel_touch_relcase(CObject* O);

	public:
		Touch();
		virtual ~Touch();

		virtual bool feel_touch_contact(CObject* O);
		virtual void feel_touch_update(Fvector& P, float R);
		virtual void feel_touch_deny(CObject* O, DWORD T);

		virtual void feel_touch_new(CObject* O)
		{
		};

		virtual void feel_touch_delete(CObject* O)
		{
		};
	};
};
