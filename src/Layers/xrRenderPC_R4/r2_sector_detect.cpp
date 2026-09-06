#include "stdafx.h"
#include "r4.h"

int CRender::translateSector(IRender_Sector* pSector)
{
	if (!pSector)
		return -1;

	for (u32 i = 0; i < Sectors.size(); ++i)
	{
		if (Sectors[i] == pSector)
			return i;
	}

	FATAL("Sector was not found!");
	NODEFAULT;

#ifdef DEBUG
	return			(-1);
#endif // #ifdef DEBUG
}

IRender_Sector* CRender::detectSector(const Fvector& P)
{
	IRender_Sector* S = NULL;
	Fvector dir;
	Sectors_xrc.ray_options(CDB::OPT_ONLYNEAREST);

	dir.set(0, -1, 0);
	S = detectSector(P, dir);
	if (NULL == S)
	{
		dir.set(0, 1, 0);
		S = detectSector(P, dir);
	}
	return S;
}

IRender_Sector* CRender::detectSector(const Fvector& P, Fvector& dir)
{
	// Portals model
	int id1 = -1;
	float range1 = 500.f;
	if (rmPortals)
	{
		Sectors_xrc.ray_query(rmPortals, P, dir, range1);
		if (Sectors_xrc.r_count())
		{
			CDB::RESULT* RP1 = Sectors_xrc.r_begin();
			id1 = RP1->id;
			range1 = RP1->range;
		}
	}

	// Geometry model
	int id2 = -1;
	float range2 = range1;
	CDB::MODEL* staticModel = g_pGameLevel->ObjectSpace.GetStaticModel();
	if (staticModel)
	{
		Sectors_xrc.ray_query(staticModel, P, dir, range2);
		if (Sectors_xrc.r_count())
		{
			CDB::RESULT* RP2 = Sectors_xrc.r_begin();
			id2 = RP2->id;
			range2 = RP2->range;
		}
	}

	// One-shot diagnostic: dump the exact state that decides whether real
	// scene geometry ever gets drawn (pLastSector never becomes non-null,
	// so render_main() permanently takes its HUD-only branch, if neither
	// query below ever finds a hit). Logged once per level load so it can't
	// spam the log across a play session.
	if (!m_sector_debug_logged)
	{
		m_sector_debug_logged = true;
		Msg("! SECTOR_DEBUG: rmPortals=%s staticModel=%s id1=%d id2=%d P=(%.2f,%.2f,%.2f) dir=(%.2f,%.2f,%.2f)",
			rmPortals ? "present" : "NULL",
			staticModel ? "present" : "NULL",
			id1, id2, P.x, P.y, P.z, dir.x, dir.y, dir.z);
	}

	// Select ID
	int ID;
	if (id1 >= 0)
	{
		if (id2 >= 0) ID = (range1 <= range2 + EPS) ? id1 : id2; // both was found
		else ID = id1; // only id1 found
	}
	else if (id2 >= 0) ID = id2; // only id2 found
	else return 0;

	if (ID == id1)
	{
		// Take sector, facing to our point from portal
		CDB::TRI* pTri = rmPortals->get_tris() + ID;
		CPortal* pPortal = (CPortal*)Portals[pTri->dummy];
		return pPortal->getSectorFacing(P);
	}
	else
	{
		// Take triangle at ID and use it's Sector
		CDB::TRI* pTri = g_pGameLevel->ObjectSpace.GetStaticTris() + ID;
		return getSector(pTri->sector);
	}
}
