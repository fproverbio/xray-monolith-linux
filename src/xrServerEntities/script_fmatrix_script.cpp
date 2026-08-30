////////////////////////////////////////////////////////////////////////////
//	Module 		: script_fmatrix_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script float matrix script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_fmatrix.h"

using namespace luabind;
using namespace luabind::policy;

Fvector get_matrix_hpb(Fmatrix* self)
{
	Fvector temp;
	self->getHPB(temp);
	return temp;
}

void matrix_transform(Fmatrix* self, Fvector* v)
{
	self->transform(*v);
}

#pragma optimize("s",on)
template<> void CScriptFmatrix::script_register(lua_State* L)
{
	module(L)
		[
			class_<Fmatrix>("matrix")
			.def_readwrite("i", &Fmatrix::i)
			.def_readwrite("_14_", &Fmatrix::_14_)
			.def_readwrite("j", &Fmatrix::j)
			.def_readwrite("_24_", &Fmatrix::_24_)
			.def_readwrite("k", &Fmatrix::k)
			.def_readwrite("_34_", &Fmatrix::_34_)
			.def_readwrite("c", &Fmatrix::c)
			.def_readwrite("_44_", &Fmatrix::_44_)
			.def(constructor<>())
			.def("set", (Fmatrix & (Fmatrix::*)(const Fmatrix&))(&Fmatrix::set), return_reference_to<1>())
			.def("set",
				(Fmatrix & (Fmatrix::*)(const Fvector&, const Fvector&, const Fvector&, const Fvector&))(&Fmatrix::set),
				return_reference_to<1>())
			.def("identity", &Fmatrix::identity, return_reference_to<1>())
			.def("mk_xform", &Fmatrix::mk_xform, return_reference_to<1>())
			.def("build_camera_dir", &Fmatrix::build_camera_dir, return_reference_to<1>())
			.def("build_projection", &Fmatrix::build_projection, return_reference_to<1>())
			.def("mulA_43", &Fmatrix::mulA_43, return_reference_to<1>())
			.def("mulA_44", &Fmatrix::mulA_44, return_reference_to<1>())
			.def("mulB_43", &Fmatrix::mulB_43, return_reference_to<1>())
			.def("mulB_44", &Fmatrix::mulB_44, return_reference_to<1>())
			.def("mul_43", &Fmatrix::mul_43, return_reference_to<1>())
			.def("translate", (Fmatrix & (Fmatrix::*)(float, float, float))(&Fmatrix::translate), return_reference_to<1>())
			.def("translate", (Fmatrix & (Fmatrix::*)(const Fvector&))(&Fmatrix::translate), return_reference_to<1>())
			.def("translate_add", (Fmatrix & (Fmatrix::*)(float, float, float))(&Fmatrix::translate_add), return_reference_to<1>())
			.def("translate_add", (Fmatrix & (Fmatrix::*)(const Fvector&))(&Fmatrix::translate_add), return_reference_to<1>())
			.def("translate_over", (Fmatrix & (Fmatrix::*)(float, float, float))(&Fmatrix::translate_over), return_reference_to<1>())
			.def("translate_over", (Fmatrix & (Fmatrix::*)(const Fvector&))(&Fmatrix::translate_over), return_reference_to<1>())
			.def("transform", (void (Fmatrix::*)(Fvector&, const Fvector&) const)(&Fmatrix::transform))
			.def("transform", (void (Fmatrix::*)(Fvector&) const)(&Fmatrix::transform))
			.def("transform_tiny", (void (Fmatrix::*)(Fvector&, const Fvector&) const)(&Fmatrix::transform_tiny))
			.def("transform_tiny", (void (Fmatrix::*)(Fvector&) const)(&Fmatrix::transform_tiny))
			.def("transform_dir", (void (Fmatrix::*)(Fvector&, const Fvector&) const)(&Fmatrix::transform_dir))
			.def("transform_dir", (void (Fmatrix::*)(Fvector&) const)(&Fmatrix::transform_dir))
			.def("mul", (Fmatrix & (Fmatrix::*)(const Fmatrix&, const Fmatrix&))(&Fmatrix::mul), return_reference_to<1>())
			.def("mul", (Fmatrix & (Fmatrix::*)(const Fmatrix&, float))(&Fmatrix::mul), return_reference_to<1>())
			.def("mul", (Fmatrix & (Fmatrix::*)(float))(&Fmatrix::mul), return_reference_to<1>())
			.def("invert", (Fmatrix & (Fmatrix::*)())(&Fmatrix::invert), return_reference_to<1>())
			.def("invert", (Fmatrix & (Fmatrix::*)(const Fmatrix&))(&Fmatrix::invert), return_reference_to<1>())
			.def("invert_b", &Fmatrix::invert_b, return_reference_to<1>())
			.def("div", (Fmatrix & (Fmatrix::*)(const Fmatrix&, float))(&Fmatrix::div), return_reference_to<1>())
			.def("div", (Fmatrix & (Fmatrix::*)(float))(&Fmatrix::div), return_reference_to<1>())
			.def("scale", (Fmatrix & (Fmatrix::*)(float, float, float))(&Fmatrix::scale), return_reference_to<1>())
			.def("scale", (Fmatrix & (Fmatrix::*)(const Fvector&))(&Fmatrix::scale), return_reference_to<1>())
			.def("setHPB", (Fmatrix & (Fmatrix::*)(float, float, float))(&Fmatrix::setHPB), return_reference_to<1>())
			.def("setHPB", (Fmatrix & (Fmatrix::*)(const Fvector&))(&Fmatrix::setHPB), return_reference_to<1>())
			.def("setXYZ", (Fmatrix & (Fmatrix::*)(float, float, float))(&Fmatrix::setXYZ), return_reference_to<1>())
			.def("setXYZ", (Fmatrix & (Fmatrix::*)(const Fvector&))(&Fmatrix::setXYZ), return_reference_to<1>())
			.def("setXYZi", (Fmatrix & (Fmatrix::*)(float, float, float))(&Fmatrix::setXYZi), return_reference_to<1>())
			.def("setXYZi", (Fmatrix & (Fmatrix::*)(const Fvector&))(&Fmatrix::setXYZi), return_reference_to<1>())
			.def("getHPB", &get_matrix_hpb)
			// hud_to_world/world_to_hud: NOT registered - instantiating
			// _matrix<T>::hud_to_world()/world_to_hud() (xrCore/_matrix.h)
			// hits a genuine pre-existing bug: their bodies call
			// Device.hud_to_world(*this) - Device is an xrEngine global
			// that xrCore, architecturally, has no business referencing at
			// all. GCC's -Wtemplate-body flags this as an unresolved name
			// the moment _matrix.h's template body is first PARSED
			// (regardless of what's #include-d later in this TU) and
			// permanently marks the template erroneous, so no amount of
			// reordering this file's own includes fixes it - real fix
			// would mean either removing the Device dependency from
			// _matrix.h itself (foundational, wide blast radius) or
			// wrapping it some other way. No xrGame/xrEngine file has ever
			// instantiated this specific method before (confirmed via
			// grep), so the bug was dormant until this registration tried
			// to. These are obscure HUD/camera-transform convenience
			// wrappers, not worth chasing a foundational xrCore fix for;
			// dropped rather than forcing it.
		];
}
