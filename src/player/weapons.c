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


// References/notes:
// [*1] http://www.quake2.com/q2wfaq/q2wfaq.html
// [*2] I need to think how spread an recoil interacts


// Keep an eye on:
//  - int client_persistant_t::inventory[MAX_ITEMS];
//        Modify this variable wrong and everything explodes,
//        luckily we don't need it too much here.

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
	uint8_t pickup_drop_ammo; // Ammo to add at pickup, to subtract at drop
	uint8_t fire_ammo;

	// Model
	const char* model_name;
	uint8_t idle_frame;

	// Behaviour
	uint8_t fire_delay;
	uint8_t muzzle_flash;

	// Projectile
	uint8_t projectiles_no; // Like pellets in a shotgun
	uint8_t damage;         // Per projectile
	uint8_t impact_effect;  // 'TE_' prefixed client effect

	float spread;            // In game units, more or less
	float spread_crouch;     // TODO [*2]
	uint8_t spread_x_recoil; // TODO [*2]

	// Recoil
	float recoil_step;         // TODO [*2]
	float recoil_restore_step; // TODO [*2]
	float max_recoil;          // TODO [*2]
};


#define NO_MUZZLE_FLASH 255
#define WEAPONS_NO 8

static struct Behaviour BEHAVIOURS[WEAPONS_NO] = {
    {
        //.print_name = "Blaster",
        .classname = "weapon_blaster",
        .ammo_classname = NULL,

        .model_name = "models/weapons/v_blast/tris.md2",
        .idle_frame = 9,

        .fire_delay = 4 - 1, // "Two shots per second" [*1]
        .muzzle_flash = MZ_BLASTER,

        .damage = (10 + 15) / 2,
        .projectiles_no = 1,
        .impact_effect = TE_BLASTER,

        .spread = 2.0f,
        .spread_crouch = 0.1f, // Almost nothing
        .spread_x_recoil = 1,

        .recoil_step = 2.5f,
        .recoil_restore_step = 0.5f,
        .max_recoil = 2.5f,
    },
    {
        //.print_name = "Shotgun",
        .classname = "weapon_shotgun",

        .ammo_classname = "ammo_shells",
        .pickup_drop_ammo = 10,
        .fire_ammo = 1,

        .model_name = "models/weapons/v_shotg/tris.md2",
        .idle_frame = 20,

        .fire_delay = 10 - 1, // "One discharge (1 shell) per second" [*1]
        .muzzle_flash = MZ_SHOTGUN,

        .damage = 4,
        .projectiles_no = 12,
        .impact_effect = TE_SHOTGUN,

        .spread = 5.0f,
        .spread_crouch = 1.0f, // Remains the same
        .spread_x_recoil = 0,

        .recoil_step = 2.5f,
        .recoil_restore_step = 0.75f,
        .max_recoil = 2.5f,
    },
    {
        //.print_name = "Machine Gun",
        .classname = "weapon_machinegun",

        .ammo_classname = "ammo_bullets",
        .pickup_drop_ammo = 50,
        .fire_ammo = 1,

        .model_name = "models/weapons/v_machn/tris.md2",
        .idle_frame = 6,

        .fire_delay = 0, // "10 bullets per second" [*1]
        .muzzle_flash = MZ_MACHINEGUN,

        .damage = 7,
        .projectiles_no = 1,
        .impact_effect = TE_GUNSHOT,

        .spread = 5.0f,
        .spread_crouch = 0.33f,
        .spread_x_recoil = 1,

        .recoil_step = 0.5f,
        .recoil_restore_step = 2.0f,
        .max_recoil = 2.5f,
    },
    {
        //.print_name = "Rocket Launcher",
        .classname = "weapon_rocketlauncher",

        .ammo_classname = "ammo_rockets",
        .pickup_drop_ammo = 5,
        .fire_ammo = 1,

        .model_name = "models/weapons/v_rocket/tris.md2",
        .idle_frame = 13,

        .fire_delay = (40 / 5) - 1, // "Five rockets every four seconds" [*1]
        .muzzle_flash = MZ_ROCKET,

        .damage = 8,
        .projectiles_no = 1,
        .impact_effect = TE_GUNSHOT,

        .recoil_step = 3.5f,
        .recoil_restore_step = 0.7f,
        .max_recoil = 3.5f,
    },
    {
        //.print_name = "Hyperblaster",
        .classname = "weapon_hyperblaster",

        .ammo_classname = "ammo_cells",
        .pickup_drop_ammo = 50,
        .fire_ammo = 1,

        .model_name = "models/weapons/v_hyperb/tris.md2",
        .idle_frame = 21,

        .fire_delay = 0, // "10 discharges (cells) per second" [*1]
        .muzzle_flash = MZ_HYPERBLASTER,

        .damage = (10 + 20) / 2,
        .projectiles_no = 1,
        .impact_effect = TE_BLASTER,

        .recoil_step = 0.2f,
        .recoil_restore_step = 0.75f,
        .max_recoil = 1.75f,
    },
    {
        //.print_name = "Railgun",
        .classname = "weapon_railgun",

        .ammo_classname = "ammo_slugs",
        .pickup_drop_ammo = 10,
        .fire_ammo = 1,

        .model_name = "models/weapons/v_rail/tris.md2",
        .idle_frame = 19,

        .fire_delay = 15 - 1, // "One shot (slug) every 1.5 seconds" [*1]
        .muzzle_flash = MZ_RAILGUN,

        .damage = (100 + 150) / 2,
        .projectiles_no = 1,
        .impact_effect = TE_RAILTRAIL,

        .recoil_step = 4.0f,
        .recoil_restore_step = 0.5f,
        .max_recoil = 4.0f,
    },
    {
        //.print_name = "BFG10K",
        .classname = "weapon_bfg",

        .ammo_classname = "ammo_cells",
        .pickup_drop_ammo = 50,
        .fire_ammo = 50,

        .model_name = "models/weapons/v_bfg/tris.md2",
        .idle_frame = 33,

        .fire_delay = 20 - 1, // "About one shot (50 cells) per 2 seconds" [*1]
        .muzzle_flash = MZ_BFG,

        .damage = 8,
        .projectiles_no = 1,
        .impact_effect = TE_GUNSHOT,

        .recoil_step = 5.0f,
        .recoil_restore_step = 0.5f,
        .max_recoil = 5.0f,
    },
    {
        //.print_name = "Hand Grenade",
        .classname = "ammo_grenades",

        .ammo_classname = "ammo_grenades", // Is its own ammo
        .pickup_drop_ammo = 5,
        .fire_ammo = 1,

        .model_name = "models/weapons/v_handgr/tris.md2",
        .idle_frame = 33,

        .fire_delay = 20 - 1, // "About one every two seconds" [*1]
        .muzzle_flash = NO_MUZZLE_FLASH,

        .damage = 8,
        .projectiles_no = 1,
        .impact_effect = TE_GUNSHOT,

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
			b = BEHAVIOURS + i;
			break;
		}
	}

	return b;
}


void PlayerNoise(edict_t* who, vec3_t where, int type) {}


// ============================================


qboolean Pickup_Weapon(edict_t* item_ent, edict_t* player_ent)
{
	// Use item classname to find an appropiate behaviour,
	// this checks if is an item of a weapon under our control
	const struct Behaviour* b = sFindBehaviour(item_ent->item->classname);
	if (b == NULL)
	{
		gi.cprintf(player_ent, PRINT_HIGH, "Pickup_Weapon(): item '%s' is not a weapon!\n", item_ent->item->classname);
		goto return_failure;
	}

	gi.cprintf(player_ent, PRINT_HIGH, "Pickup_Weapon(): '%s', index: %i\n", b->classname, ITEM_INDEX(item_ent->item));

	// Mark player persistent inventory, outside code require this variable.
	// Funny little detail, weapons accumulate in Quake 2, it's possible to carry
	// more than one, also possible to drop them
	player_ent->client->pers.inventory[ITEM_INDEX(item_ent->item)] += 1;

	// Add some ammo
	if (strcmp(b->ammo_classname, b->classname) == 0)
	{
		// Weapon is its own ammo, so instead we increase
		// how much weapons of this kind we carry
		if ((item_ent->spawnflags & ITEM_WEAPON_WITH_NO_AMMO) == 0)
			player_ent->client->pers.inventory[ITEM_INDEX(item_ent->item)] += ((int)(b->pickup_drop_ammo) - 1);
	}
	else if (b->ammo_classname != NULL)
	{
		// Ask inventory for what kind of ammo we must add
		if ((item_ent->spawnflags & ITEM_WEAPON_WITH_NO_AMMO) == 0)
		{
			gitem_t* ammo_item = FindItemByClassname(b->ammo_classname);
			Add_Ammo(player_ent, ammo_item, (int)(b->pickup_drop_ammo));
		}
		else // Oh no! a weapon with no ammo
			gi.cprintf(player_ent, PRINT_HIGH, "Weapon without ammo\n");
	}

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

	// if (prev_b == b) // TODO, works bad at levels change
	// 	return;

	// Mark player persistent weapon, outside code require this variables
	player->client->pers.lastweapon = player->client->pers.weapon;
	player->client->pers.weapon = item;

	// Mark player ammo index,
	// asking first to inventory for ammo entity index
	if (b->ammo_classname != NULL)
	{
		gitem_t* ammo_item = FindItemByClassname(b->ammo_classname);
		player->client->ammo_index = ITEM_INDEX(ammo_item);
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

void Drop_Weapon(edict_t* player, gitem_t* weapon_item)
{
	const struct Behaviour* b = sFindBehaviour(weapon_item->classname);

	if (b == NULL)
	{
		gi.cprintf(player, PRINT_HIGH, "Drop_Weapon(): item '%s' is not a weapon!\n", weapon_item->classname);
		return;
	}

	gi.cprintf(player, PRINT_HIGH, "Drop_Weapon(): '%s', index: %i\n", b->classname, ITEM_INDEX(weapon_item));

	if (player->client->pers.inventory[ITEM_INDEX(weapon_item)] <= 0)
	{
		// We already drop all weapons of this kind
		gi.cprintf(player, PRINT_HIGH, "Drop_Weapon(): no weapon '%s' in inventory\n", b->classname);
		return;
	}

	// Mark player persistent inventory
	// - For ammo, we subtract some, if possible, if not we drop a weapon tagged with 'no ammo'
	// - For weapon, we subtract one
	int tag_with;
	int ammo_index = 0;

	if (strcmp(b->ammo_classname, b->classname) != 0)
	{
		// Ammo
		if (b->ammo_classname != NULL)
		{
			gitem_t* ammo_item = FindItemByClassname(b->ammo_classname);
			ammo_index = ITEM_INDEX(ammo_item);

			if (player->client->pers.inventory[ammo_index] >= (int)(b->pickup_drop_ammo))
			{
				player->client->pers.inventory[ammo_index] -= (int)(b->pickup_drop_ammo);
				tag_with = 0;
			}
			else
				tag_with = ITEM_WEAPON_WITH_NO_AMMO;
		}

		// Weapon
		player->client->pers.inventory[ITEM_INDEX(weapon_item)] -= 1;
	}
	else
	{
		// Case where weapon is its own ammo
		ammo_index = ITEM_INDEX(weapon_item);

		if (player->client->pers.inventory[ammo_index] >= (int)(b->pickup_drop_ammo))
		{
			tag_with = 0;
			player->client->pers.inventory[ammo_index] -= (int)(b->pickup_drop_ammo);
		}
		else
		{
			tag_with = ITEM_WEAPON_WITH_NO_AMMO;
			player->client->pers.inventory[ammo_index] -= 1;
		}
	}

	// Drop weapon (spawns an item)
	edict_t* dropped_item = Drop_Item(player, weapon_item);
	dropped_item->spawnflags |= (tag_with | DROPPED_ITEM | DROPPED_PLAYER_ITEM); // Tags used in Dm to respawn items

	// Should we change current weapon?
	if (
	    // We where using it and now we have zero weapons of this kind left
	    (player->client->pers.weapon == weapon_item && player->client->pers.inventory[ITEM_INDEX(weapon_item)] == 0) ||
	    // We don't have any ammo for it
	    (ammo_index > 0 && player->client->pers.inventory[ammo_index] == 0))
	{
		Use_Weapon(player, FindItemByClassname("weapon_blaster"));
	}
}


// ============================================


static inline float sEasing(float x, float e)
{
	const float epsilon = 1.0F / 1024.0F;
	return x = (x) / (x + e * (1.0F - x) + epsilon);
}

static void sPlayNoAmmoSound(edict_t* player)
{
	// TODO: this function is similar to others in 'view.c', merge them
	if (level.time < player->pain_debounce_time)
		return;

	gi.sound(player, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
	player->pain_debounce_time = level.time + 2.0f;
}

static int sTraceRay(const edict_t* player, const struct Behaviour* b, trace_t* out, vec3_t* forward)
{
	// Calculate direction
	{
		float recoil = player->client->weapon_recoil;
		if ((player->client->ps.pmove.pm_flags & PMF_DUCKED) != 0)
			recoil *= b->spread_crouch;

		const float q = frandk() * M_PI * 2.0f; // Polar to avoid a square spread
		const float r =
		    powf(frandk(), 2.0f) * ((b->spread_x_recoil > 0) ? recoil : 1.0f) * b->spread; // Bias towards centre

		const float random_x = cos(q) * r;
		const float random_y = sin(q) * r;

		// Maths
		vec3_t angle;
		VectorSet(angle,                                 //
		          player->client->v_angle[0] + random_y, //
		          player->client->v_angle[1] + random_x, //
		          player->client->v_angle[2]);

		AngleVectors(angle, *forward, NULL, NULL);
	}

	// Trace ray
	vec3_t start;
	VectorSet(start, player->s.origin[0], player->s.origin[1], player->s.origin[2] + player->viewheight);

	vec3_t end;
	VectorMA(start, 8192, *forward, end);

	*out = gi.trace(start, NULL, NULL, end, player, MASK_SHOT);
	return 0;

	if (out->allsolid == true) // An invalid trace, player is inside a solid
		return 1;
}

static void sHitscan_(const edict_t* player, const struct Behaviour* b)
{
	int knockback = 10;
	int means_of_death = MOD_UNKNOWN;

	// Trace a ray
	vec3_t forward;
	trace_t tr;

	if (sTraceRay(player, b, &tr, &forward) != 0)
		return;

	// Impact puff
	if ((tr.surface->flags & SURF_SKY) == 0)
	{
		if (tr.ent->takedamage)
		{
			T_Damage(tr.ent, player, player, forward, tr.endpos, tr.plane.normal, (int)(b->damage), knockback,
			         DAMAGE_BULLET, means_of_death);
		}
		else
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte((int)(b->impact_effect));
			gi.WritePosition(tr.endpos);
			gi.WriteDir(tr.plane.normal);
			gi.multicast(tr.endpos, MULTICAST_PVS);
		}
	}
}

static void sHitscan(const edict_t* player, const struct Behaviour* b)
{
	for (uint8_t i = 0; i < b->projectiles_no; i += 1)
	{
		sHitscan_(player, b);
	}
}

void Think_Weapon(edict_t* player)
{
	int fire = 0;

	// Retrieve behaviour
	const struct Behaviour* b = player->client->pers.weapon->info;

	// Should we fire?
	if ((player->client->buttons & BUTTON_ATTACK) == true)
	{
		// Fire!
		if (player->client->weapon_wait < player->client->weapon_general_frame)
		{
			// No ammo
			if (b->ammo_classname != NULL &&
			    player->client->pers.inventory[player->client->ammo_index] < (int)(b->fire_ammo))
			{
				sPlayNoAmmoSound(player);
				Use_Weapon(player, FindItemByClassname("weapon_blaster"));
				return;
			}

			// Fire
			fire = 1;
			player->client->weapon_wait = player->client->weapon_general_frame + (unsigned)(b->fire_delay);
			sHitscan(player, b);

			// Subtract ammo
			if (1)
			{
				if (b->ammo_classname != NULL)
					player->client->pers.inventory[player->client->ammo_index] -= (int)(b->fire_ammo);
			}

			// Apply recoil
			player->client->weapon_recoil += b->recoil_step;
			if (player->client->weapon_recoil > b->max_recoil)
				player->client->weapon_recoil = b->max_recoil;

			// Client effect
			if (b->muzzle_flash != NO_MUZZLE_FLASH)
			{
				gi.WriteByte(svc_muzzleflash);
				gi.WriteShort(player - g_edicts);
				gi.WriteByte((int)(b->muzzle_flash));
				gi.multicast(player->s.origin, MULTICAST_PVS);
			}

			// Weapons that are its own ammo are trow away immediatly
			if (b->ammo_classname != NULL && strcmp(b->ammo_classname, b->classname) == 0 &&
			    player->client->pers.inventory[player->client->ammo_index] == 0)
			{
				Use_Weapon(player, FindItemByClassname("weapon_blaster"));
				return;
			}
		}
	}

	// We didn't fire this frame
	if (fire == 0)
	{
		// Restore recoil
		player->client->weapon_recoil -= b->recoil_restore_step;
		if (player->client->weapon_recoil < 0.0f)
			player->client->weapon_recoil = 0.0f;

		// View recoil trough an easing function
		player->client->weapon_view_recoil =
		    sEasing(player->client->weapon_recoil / b->max_recoil, 3.0f) * b->max_recoil;
	}
	else
	{
		// While firing view recoil is simply linear
		player->client->weapon_view_recoil = player->client->weapon_recoil;
	}

	// We need to trick client's model interpolation
	if (player->client->weapon_frame == 0)
		player->client->ps.gunframe = (int)(b->idle_frame);
	if (player->client->weapon_frame == 1)
		player->client->ps.gunindex = gi.modelindex(b->model_name);

	// Update state
	player->client->weapon_frame += 1;
	player->client->weapon_general_frame += 1;

	// Developers, developers, developers
	// printf("%f %s\n", level.time, (fire == 1) ? "Fire!" : "");
}
