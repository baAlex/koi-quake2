/*
 * Copyright (C) 2023 Alexander Brandt.
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
#include <stdint.h>


// References:
// [1] http://www.quake2.com/q2wfaq/q2wfaq.html


// Keep an eye on:
//  - int client_persistant_t::inventory[MAX_ITEMS];
//        Modify this variable wrong and everything explodes,
//        luckyly we don't need it too much here.

//  - gitem_t* client_persistant_t::weapon;
//        Read multiple times from outside, so it needs to be
//        valid at all times. The most problematic read is the
//        view model.

//  - gitem_t* client_persistant_t::weapon;
//        Read multiple times from outside, so it needs to be
//        valid at all times. The most problematic read is the
//        view model.

//  - gitem_t* client_persistant_t::lastweapon;
//        Only read in 'g_cmds.c', in 'weaplast' command that
//        switch between last two weapons.

// - int gclient_s::ammo_index
//        Only the Hud read this one.


struct Behaviour
{
	// const char* print_name;
	const char* classname; // Set in editor

	// Ammo
	const char* ammo_classname;

	// Model
	const char* model_name;
	uint8_t idle_frame;

	// Behaviour
	uint8_t fire_delay;
	uint8_t muzzleflash;

	// Recoil, only a visual clue
	float recoil_step;
	float recoil_restore_step;
	float max_recoil;
};


#define NO_MUZZLEFLASH 255
#define WEAPONS_NO 8

static struct Behaviour BEHAVIOURS[WEAPONS_NO] = {
    {
        //.print_name = "Blaster",
        .classname = "weapon_blaster",
        .ammo_classname = NULL,

        .model_name = "models/weapons/v_blast/tris.md2",
        .idle_frame = 9,

        .fire_delay = 5 - 1, // "Two shots per second" [1]
        .muzzleflash = MZ_BLASTER,

        .recoil_step = 2.5f,
        .recoil_restore_step = 1.75f,
        .max_recoil = 2.5f,
    },
    {
        //.print_name = "Shotgun",
        .classname = "weapon_shotgun",
        .ammo_classname = "ammo_shells",

        .model_name = "models/weapons/v_shotg/tris.md2",
        .idle_frame = 20,

        .fire_delay = 10 - 1, // "One discharge (1 shell) per second" [1]
        .muzzleflash = MZ_SHOTGUN,

        .recoil_step = 2.5f,
        .recoil_restore_step = 1.5f,
        .max_recoil = 2.5f,
    },
    {
        //.print_name = "Machine Gun",
        .classname = "weapon_machinegun",
        .ammo_classname = "ammo_bullets",

        .model_name = "models/weapons/v_machn/tris.md2",
        .idle_frame = 6,

        .fire_delay = 0, // "10 bullets per second" [1]
        .muzzleflash = MZ_MACHINEGUN,

        .recoil_step = 0.5f,
        .recoil_restore_step = 2.0f,
        .max_recoil = 2.5f,
    },
    {
        //.print_name = "Rocket Launcher",
        .classname = "weapon_rocketlauncher",
        .ammo_classname = "ammo_rockets",

        .model_name = "models/weapons/v_rocket/tris.md2",
        .idle_frame = 13,

        .fire_delay = (40 / 5) - 1, // "Five rockets every four seconds" [1]
        .muzzleflash = MZ_ROCKET,

        .recoil_step = 3.5f,
        .recoil_restore_step = 0.7f,
        .max_recoil = 3.5f,
    },
    {
        //.print_name = "Hyperblaster",
        .classname = "weapon_hyperblaster",
        .ammo_classname = "ammo_cells",

        .model_name = "models/weapons/v_hyperb/tris.md2",
        .idle_frame = 21,

        .fire_delay = 0, // "10 discharges (cells) per second" [1]
        .muzzleflash = MZ_HYPERBLASTER,

        .recoil_step = 0.2f,
        .recoil_restore_step = 0.75f,
        .max_recoil = 1.75f,
    },
    {
        //.print_name = "Railgun",
        .classname = "weapon_railgun",
        .ammo_classname = "ammo_slugs",

        .model_name = "models/weapons/v_rail/tris.md2",
        .idle_frame = 19,

        .fire_delay = 15 - 1, // "One shot (slug) every 1.5 seconds" [1]
        .muzzleflash = MZ_RAILGUN,

        .recoil_step = 4.0f,
        .recoil_restore_step = 0.5f,
        .max_recoil = 4.0f,
    },
    {
        //.print_name = "BFG10K",
        .classname = "weapon_bfg",
        .ammo_classname = "ammo_cells",

        .model_name = "models/weapons/v_bfg/tris.md2",
        .idle_frame = 33,

        .fire_delay = 20 - 1, // "About one shot (50 cells) per 2 seconds" [1]
        .muzzleflash = MZ_BFG,

        .recoil_step = 5.0f,
        .recoil_restore_step = 0.5f,
        .max_recoil = 5.0f,
    },
    {
        //.print_name = "Hand Granade",
        .classname = "ammo_grenades",
        .ammo_classname = "ammo_grenades", // Is it's own ammo

        .model_name = "models/weapons/v_handgr/tris.md2",
        .idle_frame = 33,

        .fire_delay = 20 - 1, // "About one every two seconds" [1]
        .muzzleflash = NO_MUZZLEFLASH,

        .recoil_step = 3.5f,
        .recoil_restore_step = 0.7f,
        .max_recoil = 3.5f,
    },
};


static const struct Behaviour* sFindBehaviour(const char* classname)
{
	const struct Behaviour* b = NULL;

	for (int i = 0; i < WEAPONS_NO; i += 1)
	{
		if (strcmp(classname, BEHAVIOURS[i].classname) == 0)
		{
			b = &BEHAVIOURS[i];
			break;
		}
	}

	return b;
}


void PlayerNoise(edict_t* who, vec3_t where, int type) {}


// ============================================


qboolean Pickup_Weapon(edict_t* item_ent, edict_t* player_ent)
{
	// Use item classname to find an apropiate behaviour,
	// this checks if is an item of a weapon under our control
	const struct Behaviour* b = sFindBehaviour(item_ent->item->classname);
	if (b == NULL)
	{
		gi.cprintf(player_ent, PRINT_HIGH, "Pickup_Weapon(): item '%s' is not a weapon!\n", item_ent->item->classname);
		goto return_failure;
	}

	gi.cprintf(player_ent, PRINT_HIGH, "Pickup_Weapon(): '%s', index: %i\n", b->classname, ITEM_INDEX(item_ent->item));

	// Mark player persistent inventory, outside code require this variable
	// Funny little detail, weapons accumulate in Quake 2, is possible to carry more
	// than one, also posible to drop them
	player_ent->client->pers.inventory[ITEM_INDEX(item_ent->item)] += 1;

	// Bye!
	return true; // Weapon taken

return_failure:
	return false; // Weapon not taken
}

void Use_Weapon(edict_t* player, gitem_t* item)
{
	const struct Behaviour* prev_b = player->client->pers.weapon->info;
	const struct Behaviour* b = sFindBehaviour(item->classname);

	if (b == NULL)
	{
		gi.cprintf(player, PRINT_HIGH, "Use_Weapon(): item '%s' is not a weapon!\n", item->classname);
		return;
	}

	gi.cprintf(player, PRINT_HIGH, "Use_Weapon(): '%s', index: %i\n", b->classname, ITEM_INDEX(item));

	if (prev_b == b)
		return;

	// Mark player persistent weapon, outside code require this variable
	player->client->pers.lastweapon = player->client->pers.weapon;
	player->client->pers.weapon = item;

	// Ask inventory for ammo entity index
	if (b->ammo_classname != NULL)
	{
		gitem_t* item = FindItemByClassname(b->ammo_classname);
		player->client->ammo_index = ITEM_INDEX(item);
	}
	else
		player->client->ammo_index = 0;

	// Keep the behaviour
	item->info = b;

	// Reset state, not entirely tho
	{
		// player->client->weapon_recoil -= 50.0f;
		// player->client->weapon_general_frame = 0;
		player->client->weapon_frame = 0;
		player->client->weapon_wait = 0;
		// player->client->weapon_wait2 = 0;
	}
}

void Drop_Weapon(edict_t* player, gitem_t* item)
{
	const struct Behaviour* b = sFindBehaviour(item->classname);

	if (b == NULL)
	{
		gi.cprintf(player, PRINT_HIGH, "Drop_Weapon(): item '%s' is not a weapon!\n", item->classname);
		return;
	}

	gi.cprintf(player, PRINT_HIGH, "Drop_Weapon(): '%s', index: %i\n", b->classname, ITEM_INDEX(item));
}


// ============================================


static inline float sEasing(float x, float e)
{
	const float epsilon = 1.0F / 1024.0F;
	return x = (x) / (x + e * (1.0F - x) + epsilon);
}

void Think_Weapon(edict_t* ent)
{
	int fire = 0;

	// Retrieve behaviour
	const struct Behaviour* b = ent->client->pers.weapon->info;

	// Should we fire?
	if ((ent->client->buttons & BUTTON_ATTACK) == true)
	{
		// Fire!
		if (ent->client->weapon_wait < ent->client->weapon_general_frame)
		{
			fire = 1;
			ent->client->weapon_wait = ent->client->weapon_general_frame + (unsigned)(b->fire_delay);

			// Apply recoil
			ent->client->weapon_recoil += b->recoil_step;
			if (ent->client->weapon_recoil > b->max_recoil)
				ent->client->weapon_recoil = b->max_recoil;

			// Client effect
			if (b->muzzleflash != NO_MUZZLEFLASH)
			{
				gi.WriteByte(svc_muzzleflash);
				gi.WriteShort(ent - g_edicts);
				gi.WriteByte((int)(b->muzzleflash));
				gi.multicast(ent->s.origin, MULTICAST_PVS);
			}
		}
	}

	// We didn't fire this frame
	if (fire == 0)
	{
		// Restore recoil
		ent->client->weapon_recoil -= b->recoil_restore_step;
		if (ent->client->weapon_recoil < 0.0f)
			ent->client->weapon_recoil = 0.0f;

		// View recoil trough an easing function
		ent->client->weapon_view_recoil = sEasing(ent->client->weapon_recoil / b->max_recoil, 3.0f) * b->max_recoil;
	}
	else
	{
		// While firing view recoil is simply linear
		ent->client->weapon_view_recoil = ent->client->weapon_recoil;
	}

	// We need to trick client's model interpolation
	if (ent->client->weapon_frame == 0)
		ent->client->ps.gunframe = (int)(b->idle_frame);
	if (ent->client->weapon_frame == 1)
		ent->client->ps.gunindex = gi.modelindex(b->model_name);

	// Update state
	ent->client->weapon_frame += 1;
	ent->client->weapon_general_frame += 1;

	// Developers, developers, developers
	// printf("%f %s\n", level.time, (fire == 1) ? "Fire!" : "");
}
