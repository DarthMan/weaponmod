/*
* Half-Life Weapon Mod
* Copyright (c) 2012 - 2014 AGHL.RU Dev Team
*
* http://aghl.ru/forum/ - Russian Half-Life and Adrenaline Gamer Community
*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include "wpnmod_entity.h"


CEntityManager g_Entitys;


edict_t* Wpnmod_SpawnItem(const char* szName, Vector vecOrigin, Vector vecAngles)
{
	if (strstr(szName, "ammo_"))
	{
		return Ammo_Spawn(szName, vecOrigin, vecAngles);
	}

	if (strstr(szName, "weapon_"))
	{
		return Weapon_Spawn(szName, vecOrigin, vecAngles);
	}

	return NULL;
}

edict_t* Weapon_Spawn(const char* szName, Vector vecOrigin, Vector vecAngles)
{
	int iId = 0;

	for (int i = 1; i <= g_iWeaponsCount; i++)
	{
		if (WeaponInfoArray[i].iType == Wpn_Custom && !_stricmp(GetWeapon_pszName(i), szName))
		{
			iId = i;
			break;
		}
	}

	if (!iId)
	{
		return NULL;
	}

	edict_t* pItem = CREATE_NAMED_ENTITY(MAKE_STRING("weapon_crowbar"));

	if (IsValidPev(pItem))
	{
		MDLL_Spawn(pItem);
		SET_ORIGIN(pItem, vecOrigin);

		pItem->v.classname = MAKE_STRING(GetWeapon_pszName(iId));
		pItem->v.angles = vecAngles;

		SetPrivateInt(pItem, pvData_iId, iId);

		if (GetWeapon_MaxClip(iId) != -1)
		{
			SetPrivateInt(pItem, pvData_iClip, 0);
		}

		if (WeaponInfoArray[iId].iForward[Fwd_Wpn_Spawn])
		{
			MF_ExecuteForward
			(
				WeaponInfoArray[iId].iForward[Fwd_Wpn_Spawn],

				static_cast<cell>(ENTINDEX(pItem)),
				static_cast<cell>(0),
				static_cast<cell>(0),
				static_cast<cell>(0),
				static_cast<cell>(0)
			);
		}
	}

	return pItem;
}

edict_t* Ammo_Spawn(const char* szName, Vector vecOrigin, Vector vecAngles)
{
	int iId = 0;

	for (int i = 1; i <= g_iAmmoBoxIndex; i++)
	{
		if (!_stricmp(AmmoBoxInfoArray[i].classname.c_str(), szName))
		{
			iId = i;
			break;
		}
	}

	if (!iId)
	{
		return NULL;
	}

	edict_t* pAmmoBox = CREATE_NAMED_ENTITY(MAKE_STRING("ammo_rpgclip"));

	if (IsValidPev(pAmmoBox))
	{
		MDLL_Spawn(pAmmoBox);
		SET_ORIGIN(pAmmoBox, vecOrigin);

		pAmmoBox->v.classname = MAKE_STRING(AmmoBoxInfoArray[iId].classname.c_str());
		pAmmoBox->v.angles = vecAngles;

		if (AmmoBoxInfoArray[iId].iForward[Fwd_Ammo_Spawn])
		{
			MF_ExecuteForward
			(
				AmmoBoxInfoArray[iId].iForward[Fwd_Ammo_Spawn],
				static_cast<cell>(ENTINDEX(pAmmoBox)),
				static_cast<cell>(0)
			);
		}

		SET_SIZE(pAmmoBox, Vector(-16, -16, 0), Vector(16, 16, 16));
	}

	return pAmmoBox;
}
