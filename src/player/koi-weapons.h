/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2023 Alexander Brandt
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

#include <stddef.h>
#include <stdint.h>


#define KOI_TAKE_FRAMES_NO 4 // For all weapons
#define KOI_WEAPONS_NO 6

struct koiWeaponBehaviour
{
	const char* print_name;
	const char* classname; // Set in editor

	// Ammo
	const char* ammo_classname;
	uint8_t pickup_drop_ammo; // Ammo to add at pickup, to subtract at drop
	uint8_t fire_ammo;

	// Model
	const char* model_name;
	uint8_t idle_frame;
	uint8_t take_frames[KOI_TAKE_FRAMES_NO];

	// Behaviour
	float cook_step; // 0, 1
	uint8_t fire_delay;
	uint8_t muzzle_flash;

	// Projectile
	uint8_t projectiles_no; // Like pellets in a shotgun
	uint8_t damage;         // Per projectile
	uint8_t impact_effect;  // 'TE_' prefixed client effect
	uint8_t trail_effect;   // 'TE_' prefixed client effect
	float projectiles_spray;

	// Recoil
	float recoil_step;         // 0, 1
	float recoil_restore_step; // 0, 1
	float spread;              // In angles?

	float spread_crouch_scale;
	float view_recoil_scale;
	float view_shake_scale;

	// Reload
	uint8_t magazine_size;
	const char* reload_sound_name;
	float reload_step; // 0, 1
};

enum koiWeaponStage
{
	KOI_WEAPON_TAKE,
	KOI_WEAPON_IDLE,
	// KOI_WEAPON_WARM_UP,
	KOI_WEAPON_FIRE,
	KOI_WEAPON_RELOAD,
	// KOI_WEAPON_COOLDOWN
};

struct koiWeaponState
{
	// NO POINTERS!

	size_t behaviour_index;
	size_t ammo_item_index;

	enum koiWeaponStage stage;

	unsigned restore_recoil; // 0, 1
	float recoil;            // 0, 1
	float view_recoil;       // 0, x
	float view_shake;        // 0, x

	unsigned frame;
	unsigned general_frame;

	unsigned fire_wait;
	float cook_progress;   // 0, 1
	float reload_progress; // 0, 1
	float sound_wait;
};

#include "../header/local.h"

qboolean koiWeaponPickup(struct edict_s* item_ent, struct edict_s* player_ent);
void koiWeaponUse(struct edict_s* player, struct gitem_s* weapon_item);
void koiWeaponDrop(struct edict_s* player, struct gitem_s* weapon_item);
void koiWeaponThink(struct edict_s* player);

void koiWeaponDev(const struct edict_s* player);

#endif
