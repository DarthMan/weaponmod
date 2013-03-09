/*
 * Half-Life Weapon Mod
 * Copyright (c) 2012 - 2013 AGHL.RU Dev Team
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

#include "wpnmod_grenade.h"
#include "wpnmod_config.h"
#include "wpnmod_utils.h"
#include "wpnmod_hooks.h"


#define CHECK_ENTITY(x)																\
	if (x != 0 && (FNullEnt(INDEXENT2(x)) || x < 0 || x > gpGlobals->maxEntities))	\
	{																				\
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid entity (%d).", x);				\
		return 0;																	\
	}																				\

#define CHECK_OFFSET(x)																						\
	if (x < 0 || x >= Offset_End)																			\
	{																										\
		MF_LogError(amx, AMX_ERR_NATIVE, "Function out of bounds. Got: %d  Max: %d.", x, Offset_End - 1);	\
		return 0;																							\
	}																										\

#define CHECK_OFFSET_CBASE(x)																				\
	if (x < 0 || x >= CBase_End)																			\
	{																										\
		MF_LogError(amx, AMX_ERR_NATIVE, "Function out of bounds. Got: %d  Max: %d.", x, CBase_End - 1);	\
		return 0;																							\
	}


enum e_CBase
{
	// Weapon
	CBase_pPlayer,
	CBase_pNext,

	// Player
	CBase_rgpPlayerItems,
	CBase_pActiveItem,
	CBase_pLastItem,

	CBase_End
};

enum e_Offsets
{
	// Weapon
	Offset_flStartThrow,
	Offset_flReleaseThrow,
	Offset_iChargeReady,
	Offset_iInAttack,
	Offset_iFireState,
	Offset_iFireOnEmpty,				// true when the gun is empty and the player is still holding down the attack key(s)
	Offset_flPumpTime,
	Offset_iInSpecialReload,			// Are we in the middle of a reload for the shotguns
	Offset_flNextPrimaryAttack,			// soonest time ItemPostFrame will call PrimaryAttack
	Offset_flNextSecondaryAttack,		// soonest time ItemPostFrame will call SecondaryAttack
	Offset_flTimeWeaponIdle,			// soonest time ItemPostFrame will call WeaponIdle
	Offset_iPrimaryAmmoType,			// "primary" ammo index into players m_rgAmmo[]
	Offset_iSecondaryAmmoType,			// "secondary" ammo index into players m_rgAmmo[]
	Offset_iClip,						// number of shots left in the primary weapon clip, -1 it not used
	Offset_iInReload,					// are we in the middle of a reload;
	Offset_iDefaultAmmo,				// how much ammo you get when you pick up this weapon as placed by a level designer.
	
	// Player
	Offset_flNextAttack,				// cannot attack again until this time
	Offset_iWeaponVolume,				// how loud the player's weapon is right now
	Offset_iWeaponFlash,				// brightness of the weapon flash
	Offset_iLastHitGroup,
	Offset_iFOV,

	// Custom (for weapon and "info_target" entities only)
	Offset_iuser1,
	Offset_iuser2,
	Offset_iuser3,
	Offset_iuser4,
	Offset_fuser1,
	Offset_fuser2,
	Offset_fuser3,
	Offset_fuser4,
	
	Offset_End
};

int NativesCBaseOffsets[CBase_End] =
{
	pvData_pPlayer,
	pvData_pNext,
	pvData_rgpPlayerItems,
	pvData_pActiveItem,
	pvData_pLastItem
};

int NativesPvDataOffsets[Offset_End] =
{
	pvData_flStartThrow,
	pvData_flReleaseThrow,
	pvData_chargeReady,
	pvData_fInAttack,
	pvData_fireState,
	pvData_fFireOnEmpty,
	pvData_flPumpTime,
	pvData_fInSpecialReload,
	pvData_flNextPrimaryAttack,
	pvData_flNextSecondaryAttack,
	pvData_flTimeWeaponIdle,
	pvData_iPrimaryAmmoType,
	pvData_iSecondaryAmmoType,
	pvData_iClip,
	pvData_fInReload,
	pvData_iDefaultAmmo,
	pvData_flNextAttack,
	pvData_iWeaponVolume,
	pvData_iWeaponFlash,
	pvData_LastHitGroup,
	pvData_iFOV,
	pvData_ammo_9mm,
	pvData_ammo_357,
	pvData_ammo_bolts,
	pvData_ammo_buckshot,
	pvData_ammo_rockets,
	pvData_ammo_uranium,
	pvData_ammo_hornets,
	pvData_ammo_argrens
};



/**
 * Register new weapon in module.
 *
 * @param szName		The weapon name.
 * @param iSlot			SlotID (1...5).
 * @param iPosition		NumberInSlot (1...5).
 * @param szAmmo1		Primary ammo type ("9mm", "uranium", "MY_AMMO" etc).
 * @param iMaxAmmo1		Max amount of primary ammo.
 * @param szAmmo2		Secondary ammo type.
 * @param iMaxAmmo2		Max amount of secondary ammo.
 * @param iMaxClip		Max amount of ammo in weapon's clip.
 * @param iFlags		Weapon's flags (see defines).
 * @param iWeight		This value used to determine this weapon's importance in autoselection.
 * 
 * @return				The ID of registerd weapon or 0 on failure. (integer)
 *
 * native wpnmod_register_weapon(const szName[], const iSlot, const iPosition, const szAmmo1[], const iMaxAmmo1, const szAmmo2[], const iMaxAmmo2, const iMaxClip, const iFlags, const iWeight);
*/
static cell AMX_NATIVE_CALL wpnmod_register_weapon(AMX *amx, cell *params)
{
	#define UD_FINDPLUGIN 3

	const char *szWeaponName = MF_GetAmxString(amx, params[1], 0, NULL);

	for (int i = 1; i < MAX_WEAPONS; i++)
	{
		if (WeaponInfoArray[i].iType != Wpn_None)
		{
			if (!stricmp(GetWeapon_pszName(i), szWeaponName))
			{
				MF_LogError(amx, AMX_ERR_NATIVE, "Weapon name is duplicated.");
				return -1;
			}

			if (GetWeapon_pszAmmo1(i) && GET_AMMO_INDEX(GetWeapon_pszAmmo1(i)) >= MAX_AMMO_SLOTS 
				|| GetWeapon_pszAmmo2(i) && GET_AMMO_INDEX(GetWeapon_pszAmmo2(i)) >= MAX_AMMO_SLOTS)
			{
				MF_LogError(amx, AMX_ERR_NATIVE, "Ammo limit reached.");
				return -1;
			}
		}
		else if (WeaponInfoArray[i].iType == Wpn_None)
		{
			g_iWeaponsCount++;

			WeaponInfoArray[i].iType = Wpn_Custom;

			WeaponInfoArray[i].ItemData.pszName = STRING(ALLOC_STRING(szWeaponName));
			WeaponInfoArray[i].ItemData.pszAmmo1 = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[4], 0, NULL)));
			WeaponInfoArray[i].ItemData.iMaxAmmo1 = params[5];
			WeaponInfoArray[i].ItemData.pszAmmo2 = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[6], 0, NULL)));
			WeaponInfoArray[i].ItemData.iMaxAmmo2 = params[7];
			WeaponInfoArray[i].ItemData.iMaxClip = params[8];
			WeaponInfoArray[i].ItemData.iFlags = params[9];
			WeaponInfoArray[i].ItemData.iWeight = params[10];

			CPlugin* plugin = (CPlugin*)amx->userdata[UD_FINDPLUGIN];

			WeaponInfoArray[i].title = plugin->title;
			WeaponInfoArray[i].author = plugin->author;
			WeaponInfoArray[i].version = plugin->version;

			AutoSlotDetection(i, params[2] - 1, params[3] - 1);

			if (!g_CrowbarHooksEnabled)
			{
				g_CrowbarHooksEnabled = TRUE;
		
				for (int k = 0; k < CrowbarHook_End; k++)
				{
					SetHookVirtual(&g_CrowbarHooks[k]);
				}
			}

			g_iWeaponInitID = i;
			
			UnsetHook(&g_dllFuncs[Func_PrecacheOtherWeapon]);
			PRECACHE_OTHER_WEAPON("weapon_crowbar");
			SetHook(&g_dllFuncs[Func_PrecacheOtherWeapon]);

			return i;
		}
	}

	MF_LogError(amx, AMX_ERR_NATIVE, "Weapon limit reached.");
	return -1;
}

/**
 * Register weapon's forward.
 *
 * @param iWeaponID		The ID of registered weapon.
 * @param iForward		Forward type to register.
 * @param szCallBack	The forward to call.
 *
 * native wpnmod_register_weapon_forward(const iWeaponID, const e_Forwards: iForward, const szCallBack[]);
*/
static cell AMX_NATIVE_CALL wpnmod_register_weapon_forward(AMX *amx, cell *params)
{
	int iId = params[1];

	if (iId <= 0 || iId > g_iWeaponsCount || WeaponInfoArray[iId].iType != Wpn_Custom)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid weapon id provided (%d).", iId);
		return 0;
	}

	int Fwd = params[2];

	if (Fwd < 0 || Fwd >= Fwd_Wpn_End)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Function out of bounds. Got: %d, Max: %d.", iId, Fwd_Wpn_End - 1);
		return 0;
	}

	const char *funcname = MF_GetAmxString(amx, params[3], 0, NULL);

	WeaponInfoArray[iId].iForward[Fwd] = MF_RegisterSPForwardByName
	(
		amx, 
		funcname, 
		FP_CELL, 
		FP_CELL, 
		FP_CELL, 
		FP_CELL, 
		FP_CELL, 
		FP_DONE
	);

	if (WeaponInfoArray[iId].iForward[Fwd] == -1)
	{
		WeaponInfoArray[iId].iForward[Fwd] = 0;
		MF_LogError(amx, AMX_ERR_NATIVE, "Function not found (%d, \"%s\").", Fwd, funcname);
		return 0;
	}

	return 1;
}

/**
 * Returns any ItemInfo variable for weapon. Use the e_ItemInfo_* enum.
 *
 * @param iId			The ID of registered weapon or weapon entity Id.
 * @param iInfoType		ItemInfo type.
 *
 * @return				Weapon's ItemInfo variable.
 *
 * native wpnmod_get_weapon_info(const iId, const e_ItemInfo: iInfoType, any:...);
 */
static cell AMX_NATIVE_CALL wpnmod_get_weapon_info(AMX *amx, cell *params)
{
	enum e_ItemInfo
	{
		ItemInfo_isCustom = 0,
		ItemInfo_iSlot,
		ItemInfo_iPosition,
		ItemInfo_iMaxAmmo1,
		ItemInfo_iMaxAmmo2,
		ItemInfo_iMaxClip,
		ItemInfo_iId,
		ItemInfo_iFlags,
		ItemInfo_iWeight,
		ItemInfo_szName,
		ItemInfo_szAmmo1,
		ItemInfo_szAmmo2,
		ItemInfo_szTitle,
		ItemInfo_szAuthor,
		ItemInfo_szVersion
	};

	int iId = params[1];
	int iSwitch = params[2];

	edict_t* pItem = NULL;

	if (iSwitch < ItemInfo_isCustom || iSwitch > ItemInfo_szVersion)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Undefined e_ItemInfo index: %d", iSwitch);
		return 0;
	}

	if (iId <= 0 || iId > gpGlobals->maxEntities)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid entity or weapon id provided (%d).", iId);
		return 0;
	}

	pItem = INDEXENT2(iId);

	if (IsValidPev(pItem) && strstr(STRING(pItem->v.classname), "weapon_"))
	{
		iId = GetPrivateInt(pItem, pvData_iId);
	}
	
	if (iId <= 0 || iId > g_iWeaponsCount)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid weapon id provided (%d).", iId);
		return 0;
	}

	size_t paramnum = params[0] / sizeof(cell);

	if (iSwitch >= ItemInfo_isCustom && iSwitch <= ItemInfo_iWeight && paramnum == 2)
	{
		switch (iSwitch)
		{
			case ItemInfo_isCustom:
				return WeaponInfoArray[iId].iType != Wpn_Default;

			case ItemInfo_iSlot:
				return GetWeapon_Slot(iId);

			case ItemInfo_iPosition:
				return GetWeapon_ItemPosition(iId);

			case ItemInfo_iMaxAmmo1:
				return GetWeapon_MaxAmmo1(iId);

			case ItemInfo_iMaxAmmo2:
				return GetWeapon_MaxAmmo2(iId);

			case ItemInfo_iMaxClip:
				return GetWeapon_MaxClip(iId);

			case ItemInfo_iId:
				return iId;

			case ItemInfo_iFlags:
				return GetWeapon_Flags(iId);

			case ItemInfo_iWeight:
				return GetWeapon_Weight(iId);
		}
	}
	else if (iSwitch >= ItemInfo_szName && iSwitch <= ItemInfo_szVersion && paramnum == 4)
	{
		const char* szReturnValue = NULL;

		switch (iSwitch)
		{
			case ItemInfo_szName:
				szReturnValue = GetWeapon_pszName(iId);
				break;
			case ItemInfo_szAmmo1:
				szReturnValue = GetWeapon_pszAmmo1(iId);
				break;
			case ItemInfo_szAmmo2:
				szReturnValue = GetWeapon_pszAmmo2(iId);
				break;
			case ItemInfo_szTitle:
				szReturnValue = WeaponInfoArray[iId].title.c_str();
				break;
			case ItemInfo_szAuthor:
				szReturnValue = WeaponInfoArray[iId].author.c_str();
				break;
			case ItemInfo_szVersion:
				szReturnValue = WeaponInfoArray[iId].version.c_str();
				break;
		}	

		if (!szReturnValue)
		{
			szReturnValue = "";
		}

		return MF_SetAmxString(amx, params[3], szReturnValue, params[4]);
	}

	MF_LogError(amx, AMX_ERR_NATIVE, "Unknown e_ItemInfo index or return combination %d", iSwitch);
	return 0;
}

/**
 * Returns any AmmoInfo variable for ammobox. Use the e_AmmoInfo_* enum.
 *
 * @param iId			The ID of registered ammobox or ammobox entity Id.
 * @param iInfoType		e_AmmoInfo_* type.
 *
 * @return				Ammobox's AmmoInfo variable.
 *
 * native wpnmod_get_ammobox_info(const iId, const e_AmmoInfo: iInfoType, any:...);
 */
static cell AMX_NATIVE_CALL wpnmod_get_ammobox_info(AMX *amx, cell *params)
{
	enum e_AmmoInfo
	{
		AmmoInfo_szName
	};

	int iId = params[1];
	int iSwitch = params[2];

	edict_t* pAmmoBox = NULL;

	if (iSwitch < AmmoInfo_szName || iSwitch > AmmoInfo_szName)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Undefined e_AmmoInfo index: %d", iSwitch);
		return 0;
	}

	if (iId <= 0 || iId > gpGlobals->maxEntities)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid entity or ammobox id provided (%d).", iId);
		return 0;
	}

	pAmmoBox = INDEXENT2(iId);

	if (IsValidPev(pAmmoBox) && strstr(STRING(pAmmoBox->v.classname), "ammo_"))
	{
		for (int i = 1; i <= g_iAmmoBoxIndex; i++)
		{
			if (!_stricmp(AmmoBoxInfoArray[i].classname.c_str(), STRING(pAmmoBox->v.classname)))
			{
				iId = i;
				break;
			}
		}
	}
	
	if (iId <= 0 || iId > g_iAmmoBoxIndex)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid ammobox id provided (%d).", iId);
		return 0;
	}

	size_t paramnum = params[0] / sizeof(cell);

	if (iSwitch >= AmmoInfo_szName && iSwitch <= AmmoInfo_szName && paramnum == 4)
	{
		const char* szReturnValue = NULL;

		switch (iSwitch)
		{
		case AmmoInfo_szName:
			szReturnValue = AmmoBoxInfoArray[iId].classname.c_str();
			break;
		}	

		if (!szReturnValue)
		{
			szReturnValue = "";
		}

		return MF_SetAmxString(amx, params[3], szReturnValue, params[4]);
	}

	MF_LogError(amx, AMX_ERR_NATIVE, "Unknown e_AmmoInfo index or return combination %d", iSwitch);
	return 0;
}

/**
 * Gets number of registered weapons.
 *
 * @return		Number of registered weapons. (integer)
 *
 * native wpnmod_get_weapon_count();
*/
static cell AMX_NATIVE_CALL wpnmod_get_weapon_count(AMX *amx, cell *params)
{
	return g_iWeaponsCount;
}

/**
 * Gets number of registered ammoboxes.
 *
 * @return		Number of registered ammoboxes. (integer)
 *
 * native wpnmod_get_ammobox_count();
*/
static cell AMX_NATIVE_CALL wpnmod_get_ammobox_count(AMX *amx, cell *params)
{
	return g_iAmmoBoxIndex;
}

/**
 * Plays weapon's animation.
 *
 * @param iItem		Weapon's entity.
 * @param iAnim		Sequence number.
 *
 * native wpnmod_send_weapon_anim(const iItem, const iAnim);
*/
static cell AMX_NATIVE_CALL wpnmod_send_weapon_anim(AMX *amx, cell *params)
{
	CHECK_ENTITY(params[1])

	edict_t *pWeapon = INDEXENT(params[1]);
	edict_t *pPlayer = GetPrivateCbase(pWeapon, pvData_pPlayer);

	if (!IsValidPev(pPlayer))
	{
		return 0;
	}

	SendWeaponAnim(pPlayer, pWeapon, params[2]);
	return 1;
}

/**
 * Set the activity for player based on an event or current state.
 *
 * @param iPlayer		Player id.
 * @param iPlayerAnim	Animation (See PLAYER_ANIM constants).
 *
 * native wpnmod_set_player_anim(const iPlayer, const PLAYER_ANIM: iPlayerAnim);
*/
static cell AMX_NATIVE_CALL wpnmod_set_player_anim(AMX *amx, cell *params)
{
	int iPlayer = params[1];
	int iPlayerAnim = params[2];

	CHECK_ENTITY(iPlayer)
	SET_ANIMATION(INDEXENT2(iPlayer), iPlayerAnim);
	return 1;
}

/**
 * Set animation extension for player.
 *
 * @param iPlayer		Player id.
 * @param szAnimExt[]	Animation extension prefix.
 *
 * native wpnmod_set_anim_ext(const iPlayer, const szAnimExt[]);
*/
static cell AMX_NATIVE_CALL wpnmod_set_anim_ext(AMX *amx, cell *params)
{
	int iPlayer = params[1];

	CHECK_ENTITY(iPlayer)
	SetPrivateString(INDEXENT2(iPlayer), pvData_szAnimExtention, STRING(ALLOC_STRING(MF_GetAmxString(amx, params[2], 0, NULL))));
	return 1;
}

/**
 * Get animation extension for player.
 *
 * @param iPlayer		Player id.
 * @param szDest[]		Buffer.
 * @param iMaxLen		Max buffer size.
 *
 * native wpnmod_get_anim_ext(const iPlayer, szDest[], iMaxLen);
 */
static cell AMX_NATIVE_CALL wpnmod_get_anim_ext(AMX *amx, cell *params)
{
	int iPlayer = params[1];

	CHECK_ENTITY(iPlayer)

	return MF_SetAmxString(amx, params[2], GetPrivateString(INDEXENT2(iPlayer), pvData_szAnimExtention), params[3]);
}

/**
* Get player's ammo inventory.
 *
 * @param iPlayer		Player id.
 * @param szAmmoName	Ammo type. ("9mm", "uranium", "MY_AMMO" etc..)
 *
 * @return				Amount of given ammo. (integer)
 *
 * native wpnmod_get_player_ammo(const iPlayer, const szAmmoName[]);
*/
static cell AMX_NATIVE_CALL wpnmod_get_player_ammo(AMX *amx, cell *params)
{
	int iPlayer = params[1];

	CHECK_ENTITY(iPlayer)

	int iAmmoIndex = GET_AMMO_INDEX(STRING(ALLOC_STRING(MF_GetAmxString(amx, params[2], 0, NULL))));

	if (iAmmoIndex != -1)
	{
		return GetAmmoInventory(INDEXENT2(iPlayer), iAmmoIndex);
	}

	return -1;
}

/**
* Set player's ammo inventory.
 *
 * @param iPlayer		Player id.
 * @param szAmmoName	Ammo type. ("9mm", "uranium", "MY_AMMO" etc..)
 * @param iAmount		Ammo amount.
 *
 * native wpnmod_set_player_ammo(const iPlayer, const szAmmoName[], const iAmount);
*/
static cell AMX_NATIVE_CALL wpnmod_set_player_ammo(AMX *amx, cell *params)
{
	int iPlayer = params[1];

	CHECK_ENTITY(iPlayer)

	int iAmmoIndex = GET_AMMO_INDEX(STRING(ALLOC_STRING(MF_GetAmxString(amx, params[2], 0, NULL))));

	if (iAmmoIndex != -1)
	{
		SetAmmoInventory(INDEXENT2(iPlayer), iAmmoIndex, params[3]);
		return 1;
	}

	return 0;
}

/**
 * Sets an integer from private data.
 *
 * @param iEntity		Entity index.
 * @param iOffset		Offset (See e_Offsets constants).
 * @param iValue		Value.
 *
 * native wpnmod_set_offset_int(const iEntity, const e_Offsets: iOffset, const iValue);
*/
static cell AMX_NATIVE_CALL wpnmod_set_offset_int(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iOffset = params[2];
	int iValue = params[3];

	CHECK_ENTITY(iEntity)
	CHECK_OFFSET(iOffset)

	SetPrivateInt(INDEXENT2(iEntity), NativesPvDataOffsets[iOffset], iValue);
	return 1;
}

/**
 * Returns an integer from private data.
 *
 * @param iEntity		Entity index.
 * @param iOffset		Offset (See e_Offsets constants).
 * 
 * @return				Value from private data. (integer)
 *
 * native wpnmod_get_offset_int(const iEntity, const e_Offsets: iOffset);
*/
static cell AMX_NATIVE_CALL wpnmod_get_offset_int(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iOffset = params[2];

	CHECK_ENTITY(iEntity)
	CHECK_OFFSET(iOffset)

	return GetPrivateInt(INDEXENT2(iEntity), NativesPvDataOffsets[iOffset]);
}

/**
 * Sets a float from private data.
 *
 * @param iEntity		Entity index.
 * @param iOffset		Offset (See e_Offsets constants).
 * @param flValue		Value.
 *
 * native wpnmod_set_offset_float(const iEntity, const e_Offsets: iOffset, const Float: flValue);
*/
static cell AMX_NATIVE_CALL wpnmod_set_offset_float(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iOffset = params[2];

	float flValue = amx_ctof(params[3]);

	CHECK_ENTITY(iEntity)
	CHECK_OFFSET(iOffset)
	
	SetPrivateFloat(INDEXENT2(iEntity), NativesPvDataOffsets[iOffset], flValue);
	return 1;
}

/**
 * Set the corresponding cbase field in private data with the index.
 *
 * @param iEntity			The entity to examine the private data.
 * @param iOffset			Offset (See e_CBase constants).
 * @param iValue			The index to store.
 * @param iExtraOffset		The extra offset.
 *
 * native wpnmod_set_offset_cbase(const iEntity, const e_CBase: iOffset, const iValue, const iExtraOffset = 0);
*/
static cell AMX_NATIVE_CALL wpnmod_set_offset_cbase(AMX *amx, cell *params)
{
	CHECK_ENTITY(params[1])
	CHECK_ENTITY(params[3])
	CHECK_OFFSET_CBASE(params[2])
	
	SetPrivateCbase(INDEXENT2(params[1]), NativesCBaseOffsets[params[2]], INDEXENT2(params[3]), params[4]);
	return 1;
}

/**
 * Returns a float from private data.
 *
 * @param iEntity		Entity index.
 * @param iOffset		Offset (See e_Offsets constants).
 * 
 * @return				Value from private data. (float)
 *
 * native Float: wpnmod_get_offset_float(const iEntity, const e_Offsets: iOffset);
*/
static cell AMX_NATIVE_CALL wpnmod_get_offset_float(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iOffset = params[2];

	CHECK_ENTITY(iEntity)
	CHECK_OFFSET(iOffset)

	return amx_ftoc(GetPrivateFloat(INDEXENT2(iEntity), NativesPvDataOffsets[iOffset]));
}

/**
 * This will return an index of the corresponding cbase field in private data.
 *
 * @param iEntity			The entity to examine the private data.
 * @param iOffset			Offset (See e_CBase constants).
 * @param iExtraOffset		The extra offset.
 *
 * @return					Value from private data. (integer)
 *
 * native wpnmod_get_offset_cbase(const iEntity, const e_CBase: iOffset, const iExtraOffset = 0);
*/
static cell AMX_NATIVE_CALL wpnmod_get_offset_cbase(AMX *amx, cell *params)
{
	CHECK_ENTITY(params[1])
	CHECK_OFFSET_CBASE(params[2])

	edict_t* pEntity = GetPrivateCbase(INDEXENT2(params[1]), NativesCBaseOffsets[params[2]], params[3]);

	if (IsValidPev(pEntity))
	{
		return ENTINDEX(pEntity);
	}

	return -1;
}

/**
 * Default deploy function.
 *
 * @param iItem				Weapon's entity index.
 * @param szViewModel		Weapon's view  model (V).
 * @param szWeaponModel		Weapon's player  model (P).
 * @param iAnim				Sequence number of deploy animation.
 * @param szAnimExt			Animation extension.
 *
 * native wpnmod_default_deploy(const iItem, const szViewModel[], const szWeaponModel[], const iAnim, const szAnimExt[]);
*/
static cell AMX_NATIVE_CALL wpnmod_default_deploy(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iAnim = params[4];

	const char *szViewModel = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[2], 0, NULL))); 
	const char *szWeaponModel = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[3], 0, NULL)));
	const char *szAnimExt = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[5], 0, NULL)));

	CHECK_ENTITY(iEntity)

	edict_t* pItem = INDEXENT2(iEntity);
	edict_t* pPlayer = GetPrivateCbase(pItem, pvData_pPlayer);

	if (!IsValidPev(pPlayer))
	{
		return 0;
	}
	
	if (!Weapon_CanDeploy(pItem->pvPrivateData))
	{
		return 0;
	}

	pPlayer->v.viewmodel = MAKE_STRING(szViewModel);
	pPlayer->v.weaponmodel = MAKE_STRING(szWeaponModel);

	SetPrivateString(pPlayer, pvData_szAnimExtention, szAnimExt);
	SendWeaponAnim(pPlayer, pItem, iAnim);

	SetPrivateFloat(pPlayer, pvData_flNextAttack, 0.5);
	SetPrivateFloat(pItem, pvData_flTimeWeaponIdle, 1.0);

	return 1;
}

/**
 * Default reload function.
 *
 * @param iItem				Weapon's entity index.
 * @param iClipSize			Maximum weapon's clip size.
 * @param iAnim				Sequence number of reload animation.
 * @param flDelay			Reload delay time.
 *
 * native wpnmod_default_reload(const iItem, const iClipSize, const iAnim, const Float: flDelay);
*/
static cell AMX_NATIVE_CALL wpnmod_default_reload(AMX *amx, cell *params)
{
	int iEntity = params[1];
	int iClipSize = params[2];
	int iAnim = params[3];

	float flDelay = amx_ctof(params[4]);

	CHECK_ENTITY(iEntity)

	edict_t* pItem = INDEXENT2(iEntity);
	edict_t* pPlayer = GetPrivateCbase(pItem, pvData_pPlayer);

	if (!IsValidPev(pPlayer))
	{
		return 0;
	}

	int iAmmo = GetAmmoInventory(pPlayer, PrimaryAmmoIndex(pItem));

	if (!iAmmo)
	{
		return 0;
	}

	int j = min(iClipSize - GetPrivateInt(pItem, pvData_iClip), iAmmo);

	if (!j)
	{
		return 0;
	}

	SetPrivateInt(pItem, pvData_fInReload, TRUE);
	SetPrivateFloat(pPlayer, pvData_flNextAttack, flDelay);
	SetPrivateFloat(pItem, pvData_flTimeWeaponIdle, flDelay);

	SendWeaponAnim(pPlayer, pItem, iAnim);
	return 1;
}

/**
 * Sets weapon's think function. Analogue of set_task native.
 * 
 * Usage: 
 * 	wpnmod_set_think(iItem, "M249_CompleteReload");
 * 	set_pev(iItem, pev_nextthink, get_gametime() + 1.52);
 *
 * @param iItem				Weapon's entity index.
 * @param szCallBack		The forward to call.
 *
 * native wpnmod_set_think(const iItem, const szCallBack[]);
*/
static cell AMX_NATIVE_CALL wpnmod_set_think(AMX *amx, cell *params)
{
	int iEntity = params[1];

	CHECK_ENTITY(iEntity)

	char *funcname = MF_GetAmxString(amx, params[2], 0, NULL);

	if (!strlen(funcname))
	{
		SetEntForward(INDEXENT2(iEntity), Think, NULL, NULL);
	}
	else
	{
		int iForward = MF_RegisterSPForwardByName
		(
			amx, 
			funcname, 
			FP_CELL, 
			FP_CELL, 
			FP_CELL, 
			FP_CELL, 
			FP_CELL, 
			FP_DONE
		);

		if (iForward == -1)
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Function not found (\"%s\").", funcname);
			return 0;
		}

		SetEntForward(INDEXENT2(iEntity), Think, (void*)Global_Think, iForward);
	}

	return 1;
}

/**
 * Sets entity's touch function. 
 * 
 * @param iEntity			Entity index.
 * @param szCallBack		The forward to call.
 *
 * native wpnmod_set_touch(const iEntity, const szCallBack[]);
*/
static cell AMX_NATIVE_CALL wpnmod_set_touch(AMX *amx, cell *params)
{
	int iEntity = params[1];

	CHECK_ENTITY(iEntity)

	char *funcname = MF_GetAmxString(amx, params[2], 0, NULL);

	if (!strlen(funcname))
	{
		SetEntForward(INDEXENT2(iEntity), Touch, NULL, NULL);
	}
	else
	{
		int iForward = MF_RegisterSPForwardByName
		(
			amx, 
			funcname, 
			FP_CELL, 
			FP_CELL, 
			FP_DONE
		);

		if (iForward == -1)
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Function not found (\"%s\").", funcname);
			return 0;
		}

		SetEntForward(INDEXENT2(iEntity), Touch, (void*)Global_Touch, iForward);
	}

	return 1;
}

/**
 * Fire bullets from player's weapon.
 *
 * @param iPlayer			Player index.
 * @param iAttacker			Attacker index (usualy it equal to previous param).
 * @param iShotsCount		Number of shots.
 * @param vecSpread			Spread.
 * @param flDistance		Max shot distance.
 * @param flDamage			Damage amount.
 * @param bitsDamageType	Damage type.
 * @param iTracerFreq		Tracer frequancy.
 *
 * native wpnmod_fire_bullets(const iPlayer, const iAttacker, const iShotsCount, const Float: vecSpread[3], const Float: flDistance, const Float: flDamage, const bitsDamageType, const iTracerFreq);
*/
static cell AMX_NATIVE_CALL wpnmod_fire_bullets(AMX *amx, cell *params)
{
	int iPlayer = params[1];
	int iAttacker = params[2];

	CHECK_ENTITY(iPlayer)
	CHECK_ENTITY(iAttacker)

	Vector vecSpread;

	cell *vSpread = MF_GetAmxAddr(amx, params[4]);

	vecSpread.x = amx_ctof(vSpread[0]);
	vecSpread.y = amx_ctof(vSpread[1]);
	vecSpread.z = amx_ctof(vSpread[2]);

	FireBulletsPlayer
	(
		INDEXENT2(iPlayer), 
		INDEXENT2(iAttacker), 
		params[3], 
		vecSpread, 
		amx_ctof(params[5]), 
		amx_ctof(params[6]), 
		params[7], 
		params[8]
	);

	return 1;
}

/**
 * Make damage upon entities within a certain range.
 * 	Only damage ents that can clearly be seen by the explosion.
 *
 * @param vecSrc			Origin of explosion.
 * @param iInflictor		Entity which causes the damage impact.
 * @param iAttacker			Attacker index.
 * @param flDamage			Damage amount.
 * @param flRadius			Damage radius.
 * @param iClassIgnore		Class to ignore.
 * @param bitsDamageType	Damage type (see CLASSIFY defines).
 *
 * native wpnmod_radius_damage(const Float: vecSrc[3], const iInflictor, const iAttacker, const Float: flDamage, const Float: flRadius, const iClassIgnore, const bitsDamageType);
*/
static cell AMX_NATIVE_CALL wpnmod_radius_damage(AMX *amx, cell *params)
{
	Vector vecSrc;
	cell *vSrc = MF_GetAmxAddr(amx, params[1]);

	vecSrc.x = amx_ctof(vSrc[0]);
	vecSrc.y = amx_ctof(vSrc[1]);
	vecSrc.z = amx_ctof(vSrc[2]);

	int iInflictor = params[2];
	int iAttacker = params[3];

	CHECK_ENTITY(iInflictor)
	CHECK_ENTITY(iAttacker)
	
	RADIUS_DAMAGE(vecSrc, &INDEXENT2(iInflictor)->v, &INDEXENT2(iAttacker)->v, amx_ctof(params[4]), amx_ctof(params[5]), params[6], params[7]);
	return 1;
}

/**
 * Same as wpnmod_radius_damage, but blocks 'ghost mines' and 'ghost nades'.
 *
 * @param vecSrc			Origin of explosion.
 * @param iInflictor		Entity which causes the damage impact.
 * @param iAttacker			Attacker index.
 * @param flDamage			Damage amount.
 * @param flRadius			Damage radius.
 * @param iClassIgnore		Class to ignore.
 * @param bitsDamageType	Damage type (see CLASSIFY defines).
 *
 * native wpnmod_radius_damage2(const Float: vecSrc[3], const iInflictor, const iAttacker, const Float: flDamage, const Float: flRadius, const iClassIgnore, const bitsDamageType);
*/
static cell AMX_NATIVE_CALL wpnmod_radius_damage2(AMX *amx, cell *params)
{
	Vector vecSrc;
	cell *vSrc = MF_GetAmxAddr(amx, params[1]);

	vecSrc.x = amx_ctof(vSrc[0]);
	vecSrc.y = amx_ctof(vSrc[1]);
	vecSrc.z = amx_ctof(vSrc[2]);

	int iInflictor = params[2];
	int iAttacker = params[3];

	CHECK_ENTITY(iInflictor)
	CHECK_ENTITY(iAttacker)

	RadiusDamage2(vecSrc, INDEXENT2(iInflictor), INDEXENT2(iAttacker), amx_ctof(params[4]), amx_ctof(params[5]), params[6], params[7]);
	return 1;
}

/**
 * Eject a brass from player's weapon.
 *
 * @param iPlayer			Player index.
 * @param iShellModelIndex	Index of precached shell's model.
 * @param iSoundtype		Bounce sound type (see defines).
 * @param flForwardScale	Forward scale value.
 * @param flUpScale			Up scale value.
 * @param flRightScale		Right scale value.
 *
 * native wpnmod_eject_brass(const iPlayer, const iShellModelIndex, const iSoundtype, const Float: flForwardScale, const Float: flUpScale, const Float: flRightScale);
*/
static cell AMX_NATIVE_CALL wpnmod_eject_brass(AMX *amx, cell *params)
{
	int iPlayer = params[1];

	CHECK_ENTITY(iPlayer)

	edict_t* pPlayer = INDEXENT2(iPlayer);

	Vector vecShellVelocity = pPlayer->v.velocity + gpGlobals->v_right * RANDOM_FLOAT(50, 70) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;
	
	UTIL_EjectBrass
	(
		pPlayer->v.origin + pPlayer->v.view_ofs + gpGlobals->v_up * amx_ctof(params[5]) + gpGlobals->v_forward * amx_ctof(params[4]) + gpGlobals->v_right * amx_ctof(params[6]), 
		vecShellVelocity, 
		pPlayer->v.angles.y, 
		params[2], 
		params[3]
	);

	return 1;
}

/**
 * Sets the weapon so that it can play empty sound again.
 *
 * @param iItem				Weapon's entity index.
 *
 * native wpnmod_reset_empty_sound(const iItem);
*/
static cell AMX_NATIVE_CALL wpnmod_reset_empty_sound(AMX *amx, cell *params)
{
	int iEntity = params[1];

	CHECK_ENTITY(iEntity)
	SetPrivateInt(INDEXENT2(iEntity), pvData_iPlayEmptySound, TRUE);
	return 1;
}

/**
 * Plays the weapon's empty sound.
 *
 * @param iItem				Weapon's entity index.
 *
 * native wpnmod_play_empty_sound(const iItem);
*/
static cell AMX_NATIVE_CALL wpnmod_play_empty_sound(AMX *amx, cell *params)
{
	int iEntity = params[1];
	CHECK_ENTITY(iEntity)

	if (GetPrivateInt(INDEXENT2(iEntity), pvData_iPlayEmptySound))
	{
		edict_t* pPlayer = GetPrivateCbase(INDEXENT2(iEntity), pvData_pPlayer);

		if (IsValidPev(pPlayer))
		{
			EMIT_SOUND_DYN2(pPlayer, CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM, 0, PITCH_NORM);
			SetPrivateInt(INDEXENT2(iEntity), pvData_iPlayEmptySound, FALSE);
			
			return 1;
		}
	}

	return 0;
}

/**
 * Spawn an item by name.
 *
 * @param szName			Item's name.
 * @param vecOrigin			Origin were to spawn.
 * @param vecAngles			Angles.
 *
 * @return					Item entity index or -1 on failure. (integer)
 *
 * native wpnmod_create_item(const szName[], const Float: vecOrigin[3] = {0.0, 0.0, 0.0}, const Float: vecAngles[3] = {0.0, 0.0, 0.0});
*/
static cell AMX_NATIVE_CALL wpnmod_create_item(AMX *amx, cell *params)
{
	char *itemname = MF_GetAmxString(amx, params[1], 0, NULL);

	Vector vecOrigin;
	cell *vOrigin = MF_GetAmxAddr(amx, params[2]);

	vecOrigin.x = amx_ctof(vOrigin[0]);
	vecOrigin.y = amx_ctof(vOrigin[1]);
	vecOrigin.z = amx_ctof(vOrigin[2]);

	Vector vecAngles;
	cell *vAngles = MF_GetAmxAddr(amx, params[3]);

	vecAngles.x = amx_ctof(vAngles[0]);
	vecAngles.y = amx_ctof(vAngles[1]);
	vecAngles.z = amx_ctof(vAngles[2]);

	edict_t* iItem = Weapon_Spawn(itemname, vecOrigin, vecAngles);

	if (IsValidPev(iItem))
	{
		return ENTINDEX(iItem);
	}
	else
	{
		edict_t* iItem = Ammo_Spawn(itemname, vecOrigin, vecAngles);

		if (IsValidPev(iItem))
		{
			return ENTINDEX(iItem);
		}	
	}

	return -1;
}

/**
 * Register new ammobox in module.
 *
 * @param szName			The ammobox classname.
 * 
 * @return					The ID of registerd ammobox or -1 on failure. (integer)
 *
 * native wpnmod_register_ammobox(const szClassname[]);
 */
static cell AMX_NATIVE_CALL wpnmod_register_ammobox(AMX *amx, cell *params)
{
	if (g_iAmmoBoxIndex >= MAX_WEAPONS)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Ammobox limit reached.");
		return -1;
	}

	if (!g_AmmoBoxHooksEnabled)
	{
		g_AmmoBoxHooksEnabled = TRUE;
		SetHookVirtual(&g_RpgAddAmmo_Hook);
	}

	const char *szAmmoboxName = MF_GetAmxString(amx, params[1], 0, NULL);

	for (int i = 1; i <= g_iAmmoBoxIndex; i++)
	{
		if (!_stricmp(AmmoBoxInfoArray[i].classname.c_str(), szAmmoboxName))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "Ammobox name is duplicated.");
			return -1;
		}
	}

	AmmoBoxInfoArray[++g_iAmmoBoxIndex].classname.assign(STRING(ALLOC_STRING(szAmmoboxName)));
	return g_iAmmoBoxIndex;
}

/**
 * Register ammobox's forward.
 *
 * @param iAmmoboxID		The ID of registered ammobox.
 * @param iForward			Forward type to register.
 * @param szCallBack		The forward to call.
 *
 * native wpnmod_register_ammobox_forward(const iWeaponID, const e_AmmoFwds: iForward, const szCallBack[]);
 */
static cell AMX_NATIVE_CALL wpnmod_register_ammobox_forward(AMX *amx, cell *params)
{
	int iId = params[1];

	if (iId <= 0 || iId > g_iAmmoBoxIndex)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Invalid ammobox id provided (%d).", iId);
		return 0;
	}

	int Fwd = params[2];

	if (Fwd < 0 || Fwd >= Fwd_Ammo_End)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "Function out of bounds. Got: %d, Max: %d.", iId, Fwd_Ammo_End - 1);
		return 0;
	}

	const char *funcname = MF_GetAmxString(amx, params[3], 0, NULL);

	AmmoBoxInfoArray[iId].iForward[Fwd] = MF_RegisterSPForwardByName
	(
		amx, 
		funcname, 
		FP_CELL,
		FP_CELL,
		FP_DONE
	);

	if (AmmoBoxInfoArray[iId].iForward[Fwd] == -1)
	{
		AmmoBoxInfoArray[iId].iForward[Fwd] = 0;
		MF_LogError(amx, AMX_ERR_NATIVE, "Function not found (%d, \"%s\").", Fwd, funcname);
		return 0;
	}

	return 1;
}

/**
 * Resets the global multi damage accumulator.
 *
 * native wpnmod_clear_multi_damage();
 */
static cell AMX_NATIVE_CALL wpnmod_clear_multi_damage(AMX *amx, cell *params)
{
	CLEAR_MULTI_DAMAGE();
	return 1;
}

/**
 * Inflicts contents of global multi damage register on entity.
 *
 * @param iInflictor		Entity which causes the damage impact.
 * @param iAttacker			Attacker index.
 *
 * native wpnmod_apply_multi_damage(const iInflictor, const iAttacker);
 */
static cell AMX_NATIVE_CALL wpnmod_apply_multi_damage(AMX *amx, cell *params)
{
	CHECK_ENTITY(params[1])
	CHECK_ENTITY(params[2])

	APPLY_MULTI_DAMAGE(INDEXENT2(params[1]), INDEXENT2(params[2]));
	return 1;
}

/**
 * Returns index of random damage decal for given entity.
 *
 * @param iEntity		Entity.
 *
 * @return				Index of damage decal. (integer)
 *
 * native wpnmod_get_damage_decal(const iEntity);
 */
static cell AMX_NATIVE_CALL wpnmod_get_damage_decal(AMX *amx, cell *params)
{
	CHECK_ENTITY(params[1])

	int decalNumber = GET_DAMAGE_DECAL(INDEXENT2(params[1]));

	if (decalNumber < 0 || decalNumber > (int)g_Decals.size())
	{
		return -1;
	}
	
	return g_Decals[decalNumber]->index;
}

/**
 * Fire default contact grenade from player's weapon.
 *
 * @param iPlayer			Player index.
 * @param vecStart			Start position.
 * @param vecVelocity		Velocity.
 * @param szCallBack		The forward to call on explode.
 *
 * @return					Contact grenade index or -1 on failure. (integer)
 *
 * native wpnmod_fire_contact_grenade(const iPlayer, const Float: vecStart[3], const Float: vecVelocity[3], const szCallBack[] = "");
*/
static cell AMX_NATIVE_CALL wpnmod_fire_contact_grenade(AMX *amx, cell *params)
{
	CHECK_ENTITY(params[1])

	Vector vecStart;
	Vector vecVelocity;

	cell *vStart = MF_GetAmxAddr(amx, params[2]);
	cell *vVelocity = MF_GetAmxAddr(amx, params[3]);

	vecStart.x = amx_ctof(vStart[0]);
	vecStart.y = amx_ctof(vStart[1]);
	vecStart.z = amx_ctof(vStart[2]);

	vecVelocity.x = amx_ctof(vVelocity[0]);
	vecVelocity.y = amx_ctof(vVelocity[1]);
	vecVelocity.z = amx_ctof(vVelocity[2]);

	edict_t* pGrenade = Grenade_ShootContact(INDEXENT2(params[1]), vecStart, vecVelocity);

	if (IsValidPev(pGrenade))
	{
		char *funcname = MF_GetAmxString(amx, params[4], 0, NULL);

		if (funcname)
		{
			int iForward = MF_RegisterSPForwardByName
			(
				amx, 
				funcname, 
				FP_CELL,
				FP_CELL,
				FP_DONE
			);

			if (iForward != -1)
			{
				g_Ents[ENTINDEX(pGrenade)].iExplode = iForward;
			}
		}

		return ENTINDEX(pGrenade);
	}

	return -1;
}

/**
 * Fire default timed grenade from player's weapon.
 *
 * @param iPlayer			Player index.
 * @param vecStart			Start position.
 * @param vecVelocity		Velocity.
 * @param flTime			Time before detonate.
 * @param szCallBack		The forward to call on explode.
 *
 * @return					Contact grenade index or -1 on failure. (integer)
 *
 * native wpnmod_fire_timed_grenade(const iPlayer, const Float: vecStart[3], const Float: vecVelocity[3], const Float: flTime = 3.0, const szCallBack[] = "");
*/
static cell AMX_NATIVE_CALL wpnmod_fire_timed_grenade(AMX *amx, cell *params)
{
	CHECK_ENTITY(params[1])

	Vector vecStart;
	Vector vecVelocity;

	cell *vStart = MF_GetAmxAddr(amx, params[2]);
	cell *vVelocity = MF_GetAmxAddr(amx, params[3]);

	vecStart.x = amx_ctof(vStart[0]);
	vecStart.y = amx_ctof(vStart[1]);
	vecStart.z = amx_ctof(vStart[2]);

	vecVelocity.x = amx_ctof(vVelocity[0]);
	vecVelocity.y = amx_ctof(vVelocity[1]);
	vecVelocity.z = amx_ctof(vVelocity[2]);

	edict_t* pGrenade = Grenade_ShootTimed(INDEXENT2(params[1]), vecStart, vecVelocity, amx_ctof(params[4]));

	if (IsValidPev(pGrenade))
	{
		char *funcname = MF_GetAmxString(amx, params[5], 0, NULL);

		if (funcname)
		{
			int iForward = MF_RegisterSPForwardByName
			(
				amx, 
				funcname, 
				FP_CELL,
				FP_CELL,
				FP_DONE
			);

			if (iForward != -1)
			{
				g_Ents[ENTINDEX(pGrenade)].iExplode = iForward;
			}
		}

		return ENTINDEX(pGrenade);
	}

	return -1;
}

/**
 * Get player's gun position. Result will set in vecResult.
 *
 * @param iPlayer			Player index.
 * @param vecResult			Calculated gun position.
 * @param flForwardScale	Forward scale value.
 * @param flUpScale			Up scale value.
 * @param flRightScale		Right scale value.
 *
 * native wpnmod_get_gun_position(const iPlayer, Float: vecResult[3], const Float: flForwardScale = 1.0, const Float: flRightScale = 1.0, const Float: flUpScale = 1.0);
*/
static cell AMX_NATIVE_CALL wpnmod_get_gun_position(AMX *amx, cell *params)
{
	CHECK_ENTITY(params[1])
	edict_t* pPlayer = INDEXENT2(params[1]);

	Vector vecSrc = pPlayer->v.origin + pPlayer->v.view_ofs; 
	MAKE_VECTORS(pPlayer->v.v_angle + pPlayer->v.punchangle);

	Vector vecForward = gpGlobals->v_forward;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;

	vecSrc = vecSrc + vecForward * amx_ctof(params[3]) + vecRight * amx_ctof(params[4]) + vecUp * amx_ctof(params[5]);
	cell* cRet = MF_GetAmxAddr(amx, params[2]);

	cRet[0] = amx_ftoc(vecSrc.x);
	cRet[1] = amx_ftoc(vecSrc.y);
	cRet[2] = amx_ftoc(vecSrc.z);

	return 1;
}

/**
 * Explode and then remove entity.
 *
 * @param iEntity			Entity index.
 * @param bitsDamageType	Damage type (see CLASSIFY defines).
 * @param szCallBack		The forward to call on explode.
 *
 * native wpnmod_explode_entity(const iEntity, const bitsDamageType = 0, const szCallBack[] = "");
*/
static cell AMX_NATIVE_CALL wpnmod_explode_entity(AMX *amx, cell *params)
{
	CHECK_ENTITY(params[1])

	char *funcname = MF_GetAmxString(amx, params[3], 0, NULL);

	if (funcname)
	{
		int iForward = MF_RegisterSPForwardByName
		(
			amx, 
			funcname, 
			FP_CELL,
			FP_CELL,
			FP_DONE
		);

		if (iForward != -1)
		{
			g_Ents[params[1]].iExplode = iForward;
		}
	}

	Grenade_Explode(INDEXENT2(params[1]), params[2]);
	return 1;
}

/**
 * Draw decal by index or name on trace end.
 *
 * @param iTrace			Trace handler.
 * @param iDecalIndex		Decal index.
 * @param szDecalName		Decal name.
 *
 * native wpnmod_decal_trace(const iTrace, const iDecalIndex = -1, const szDecalName[] = "");
*/
static cell AMX_NATIVE_CALL wpnmod_decal_trace(AMX *amx, cell *params)
{
	TraceResult *pTrace = reinterpret_cast<TraceResult *>(params[1]);

	int iDecalIndex = params[2];

	if (iDecalIndex == -1)
	{
		iDecalIndex = DECAL_INDEX(MF_GetAmxString(amx, params[3], 0, NULL));
	}

	UTIL_DecalTrace(pTrace, iDecalIndex);
	return 1;
}

/**
 * Detects the texture of an entity from a direction.
 *
 * @param iEntity			Entity index that we want to get the texture.
 * @param vecSrc			The point from where the trace starts.
 * @param vecEnd			The point where the trace ends.
 * @param szTextureName		Buffer to save the texture name.
 * @param iLen				Buffer's length.
 *
 * native wpnmod_trace_texture(const iEntity, const Float: vecSrc[3], const Float: vecEnd[3], szTextureName[], const iLen);
*/
static cell AMX_NATIVE_CALL wpnmod_trace_texture(AMX *amx, cell *params)
{
	Vector vecSrc;
	Vector vecEnd;

	edict_t *pEntity = NULL;

	if (!params[1])
	{
		pEntity = ENT(0);
	}
	else
	{
		CHECK_ENTITY(params[1])
		pEntity = INDEXENT2(params[1]);
	}

	cell *vSrc = MF_GetAmxAddr(amx, params[2]);
	cell *vEnd = MF_GetAmxAddr(amx, params[3]);

	vecSrc.x = amx_ctof(vSrc[0]);
	vecSrc.y = amx_ctof(vSrc[1]);
	vecSrc.z = amx_ctof(vSrc[2]);

	vecEnd.x = amx_ctof(vEnd[0]);
	vecEnd.y = amx_ctof(vEnd[1]);
	vecEnd.z = amx_ctof(vEnd[2]);

	const char *pTextureName = TRACE_TEXTURE(pEntity, vecSrc, vecEnd);

	if (!pTextureName)
	{
		pTextureName = "";
	}

	return MF_SetAmxString(amx, params[4], pTextureName, params[5]);
}


AMX_NATIVE_INFO Natives[] = 
{
	{ "wpnmod_register_weapon", wpnmod_register_weapon},
	{ "wpnmod_register_weapon_forward", wpnmod_register_weapon_forward},
	{ "wpnmod_register_ammobox", wpnmod_register_ammobox},
	{ "wpnmod_register_ammobox_forward", wpnmod_register_ammobox_forward},
	{ "wpnmod_get_weapon_info", wpnmod_get_weapon_info},
	{ "wpnmod_get_ammobox_info", wpnmod_get_ammobox_info},
	{ "wpnmod_get_weapon_count", wpnmod_get_weapon_count},
	{ "wpnmod_get_ammobox_count", wpnmod_get_ammobox_count},
	{ "wpnmod_send_weapon_anim", wpnmod_send_weapon_anim},
	{ "wpnmod_set_player_anim", wpnmod_set_player_anim},
	{ "wpnmod_set_anim_ext", wpnmod_set_anim_ext},
	{ "wpnmod_get_anim_ext", wpnmod_get_anim_ext},
	{ "wpnmod_set_think", wpnmod_set_think},
	{ "wpnmod_set_touch", wpnmod_set_touch},
	{ "wpnmod_set_offset_int", wpnmod_set_offset_int},
	{ "wpnmod_set_offset_float", wpnmod_set_offset_float},
	{ "wpnmod_set_offset_cbase", wpnmod_set_offset_cbase},
	{ "wpnmod_get_offset_int", wpnmod_get_offset_int},
	{ "wpnmod_get_offset_float", wpnmod_get_offset_float},
	{ "wpnmod_get_offset_cbase", wpnmod_get_offset_cbase},
	{ "wpnmod_get_player_ammo", wpnmod_get_player_ammo},
	{ "wpnmod_set_player_ammo", wpnmod_set_player_ammo},
	{ "wpnmod_default_deploy", wpnmod_default_deploy},
	{ "wpnmod_default_reload", wpnmod_default_reload},
	{ "wpnmod_fire_bullets", wpnmod_fire_bullets},
	{ "wpnmod_fire_contact_grenade", wpnmod_fire_contact_grenade},
	{ "wpnmod_fire_timed_grenade", wpnmod_fire_timed_grenade},
	{ "wpnmod_radius_damage", wpnmod_radius_damage},
	{ "wpnmod_radius_damage2", wpnmod_radius_damage2},
	{ "wpnmod_clear_multi_damage", wpnmod_clear_multi_damage},
	{ "wpnmod_apply_multi_damage", wpnmod_apply_multi_damage},
	{ "wpnmod_eject_brass", wpnmod_eject_brass},
	{ "wpnmod_reset_empty_sound", wpnmod_reset_empty_sound},
	{ "wpnmod_play_empty_sound", wpnmod_play_empty_sound},
	{ "wpnmod_create_item", wpnmod_create_item},
	{ "wpnmod_get_damage_decal", wpnmod_get_damage_decal},
	{ "wpnmod_get_gun_position", wpnmod_get_gun_position},
	{ "wpnmod_explode_entity", wpnmod_explode_entity},
	{ "wpnmod_decal_trace", wpnmod_decal_trace},
	{ "wpnmod_trace_texture", wpnmod_trace_texture},

	{ NULL, NULL }
};