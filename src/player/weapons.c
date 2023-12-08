/*
 * Copyright (C) 1997-2001 Id Software, Inc.
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

#include "../header/local.h"

struct WeaponBehaviour
{
	const char* id_name;
	const char* print_name;
	const char* model_name;
};

#define WEAPONS_NO 8
static struct WeaponBehaviour BEHAVIOURS[WEAPONS_NO] = {
    {
        .id_name = "blaster",
        .print_name = "Blaster",
        .model_name = "models/weapons/v_blast/tris.md2",
    },
    {
        .id_name = "shotgun",
        .print_name = "Shotgun",
        .model_name = "models/weapons/v_shotg/tris.md2",
    },
    {
        .id_name = "machine_gun",
        .print_name = "Machine Gun",
        .model_name = "models/weapons/v_machn/tris.md2",
    },
    {
        .id_name = "hyperblaster",
        .print_name = "Hyperblaster",
        .model_name = "models/weapons/v_hyperb/tris.md2",
    },
    {
        .id_name = "rocket_launcher",
        .print_name = "Rocket Launcher",
        .model_name = "models/weapons/v_rocket/tris.md2",
    },
    {
        .id_name = "railgun",
        .print_name = "Railgun",
        .model_name = "models/weapons/v_rail/tris.md2",
    },
    {
        .id_name = "bfg10k",
        .print_name = "BFG10k",
        .model_name = "models/weapons/v_bfg/tris.md2",
    },
    {
        .id_name = "hand_granade",
        .print_name = "Hand Granade",
        .model_name = "models/weapons/v_handgr/tris.md2",
    },
};


void PlayerNoise(edict_t* who, vec3_t where, int type) {}

qboolean Pickup_Weapon(edict_t* ent, edict_t* other)
{
	return true; // Return that was taken
}

void ChangeWeapon(edict_t* ent) {}

void Think_Weapon(edict_t* ent) {}

void Use_Weapon(edict_t* ent, gitem_t* item) {}

void Drop_Weapon(edict_t* ent, gitem_t* item) {}
