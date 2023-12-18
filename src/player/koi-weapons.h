/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2023 Alexander Brandt.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef KOI_WEAPONS_H
#define KOI_WEAPONS_H

struct koiWeaponBehaviour;

struct koiWeaponState
{
	struct gitem_s* ammo_item; // May be NULL

	const struct koiWeaponBehaviour* b;
	float recoil;
	unsigned frame;
	unsigned general_frame;
	unsigned wait;
};

#include "../header/local.h"

qboolean koiWeaponPickup(struct edict_s* item_ent, struct edict_s* player_ent);
void koiWeaponUse(struct edict_s* player, struct gitem_s* weapon_item);
void koiWeaponDrop(struct edict_s* player, struct gitem_s* weapon_item);
void koiWeaponThink(struct edict_s* player);

#endif
