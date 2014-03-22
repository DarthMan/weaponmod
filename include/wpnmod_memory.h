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

#ifndef _WPNMOD_MEMORY_H
#define _WPNMOD_MEMORY_H

#include "amxxmodule.h"
#include "wpnmod_hooker.h"
#include "wpnmod_hooks.h"


#ifdef __linux__
	#define SERVER_OS "Linux"
#else
	#define SERVER_OS "Windows"
#endif


class CMemory
{
private:
	size_t	m_start;
	size_t	m_end;
	module	m_GameDllModule;
	bool	m_bSuccess;

public:
	CMemory();

	bool FindFuncsInDll(void);

	void Parse_ClearMultiDamage(void);
	void Parse_ApplyMultiDamage(void);
	void Parse_PrecacheOtherWeapon(void);
	void Parse_GetAmmoIndex(void);
	void Parse_GiveNamedItem(void);
	void Parse_SetAnimation(void);
	void Parse_SubRemove(void);

	size_t ParseFunc(size_t start, size_t end, char* funcname, unsigned char* pattern, char* mask, size_t bytes);
	size_t ParseFunc(size_t start, size_t end, char* funcname, char* string, unsigned char* pattern, char* mask, size_t bytes);

	void* m_pSubRemove;
	void* m_WpnBoxKillThink;
};

extern CMemory g_Memory;


















#ifdef __linux__
	extern bool g_bNewGCC;
#endif

extern void* g_pAdress_SubRemove;
extern void* g_pAdress_WpnBoxKillThink;

void	EnableWeaponboxModels		(void);
void	EnableShieldHitboxTracing	(void);


#endif // _WPNMOD_MEMORY_H
