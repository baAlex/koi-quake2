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

#include "koi-weapons.h"


// References/notes:
// [*1] http://www.quake2.com/q2wfaq/q2wfaq.html
// [*2] Me... actually reading the code


// Keep an eye on:
//  - int client_persistant_t::inventory[MAX_ITEMS];
//        Modify this variable wrong and everything explodes,
//        luckily we don't need it too much here.

//  - struct gitem_s* client_persistant_t::weapon;
//        Read multiple times from outside, so it needs to be
//        valid at all times. The most problematic read is the
//        view model.

//  - struct gitem_s* client_persistant_t::lastweapon;
//        Only read in 'g_cmds.c', in 'weaplast' command that
//        switch between last two weapons.

// Also I should honor follow cvars:
//  - [    ] dmflags: DF_WEAPONS_STAY
//  - [done] dmflags: DF_INFINITE_AMMO
//  - [    ] coop
//  - [    ] coop_pickup_weapons
//  - [done] g_select_empty

// Probably not this ones:
//  - [    ] deathmatch (changes weapons damage)
//  - [    ] g_swap_speed (seems to be a Yamagi addition)

// And finally, if there is Quad damage and Silencer, powerups


#define NO_MUZZLE_FLASH 255
#define NO_TRAIL 255

static struct koiWeaponBehaviour BEHAVIOURS[KOI_WEAPONS_NO] = {
    {
        .print_name = "Blaster",
        .classname = "weapon_blaster",
        .fire_ammo = 0,

        .model_name = "models/weapons/v_blast/tris.md2",
        .idle_frame = 9,
        .take_frames = {0, 1, 2, 3},

        .cook_step = 0.0f,
        //.fire_delay = 5 - 1, // "Two shots per second" [*1]
        .fire_delay = 4 - 1, // "Two shots per second" [*1]
        .muzzle_flash = MZ_BLASTER,
        .trail_effect = NO_TRAIL,

        .damage = 10, // "10 in single play, 15 in deathmatch" [*1]
        .projectiles_no = 1,
        .impact_effect = TE_BLASTER,

        .recoil_step = 1.0f,
        .recoil_restore_step = 1.0f / (5.0f - 1.0f), // Match old fire delay, newer is faster
        .spread = 7.0f,
        .spread_crouch_scale = 0.1f, // Almost no spread
        .view_recoil_scale = 3.0f,
        .view_shake_scale = 1.0f,
    },
    {
        .print_name = "Shotgun",
        .classname = "weapon_shotgun",

        .ammo_classname = "ammo_shells",
        .pickup_drop_ammo = 10, // "10 Per Box" [*1]
        .fire_ammo = 1,

        .model_name = "models/weapons/v_shotg/tris.md2",
        .idle_frame = 21,
        .take_frames = {0, 1, 6, 7},

        .cook_step = 0.0f,
        //.fire_delay = 10 - 1, // "One discharge (1 shell) per second" [*1]
        .fire_delay = 9 - 1, // A bit faster
        .muzzle_flash = MZ_SHOTGUN,
        .trail_effect = NO_TRAIL,

        .damage = 4, // "4 per pellet, 12 pellets" [*1]
        .projectiles_no = 12,
        .impact_effect = TE_SHOTGUN,
        .projectiles_spray = 10.0f,

        .recoil_step = 1.0f,
        .recoil_restore_step = 1.0f / (10.0f - 1.0f), // Match old fire delay, newer is faster
        .spread = 50.0f,                              // Up to 100 is tolerable
        .spread_crouch_scale = 0.2f,
        .view_recoil_scale = 4.0f,
        .view_shake_scale = 2.0f,

        .magazine_size = 10,
        .reload_sound_name = "weapons/shotgre.wav",
        .reload_step = 1.0f / (44.0f),
    },
    {
        .print_name = "Machine Gun",
        .classname = "weapon_machinegun",

        .ammo_classname = "ammo_bullets",
        .pickup_drop_ammo = 50, // "50 Per Box" [*1]
        .fire_ammo = 1,

        .model_name = "models/weapons/v_machn/tris.md2",
        .idle_frame = 6,
        .take_frames = {0, 1, 2, 3},

        .cook_step = 0.0f,
        .fire_delay = 0, // "10 bullets per second" [*1]
        .muzzle_flash = MZ_MACHINEGUN,
        .trail_effect = NO_TRAIL,

        .damage = 7, // "8 per bullet" [*1]
        .projectiles_no = 1,
        .impact_effect = TE_GUNSHOT,

        .recoil_step = (1.0f) / 12.0f, // 12 shots
        //.recoil_restore_step = 0.1f, // Fine, a bit slow
        //.recoil_restore_step = 0.2f, // Fast, quite good maybe too much
        .recoil_restore_step = 0.15f,
        .spread = 12.0f,
        .spread_crouch_scale = 0.6f,
        .view_recoil_scale = 7.0f,
        .view_shake_scale = 1.8f,

        .magazine_size = 50,
        .reload_sound_name = "weapons/machgre.wav",
        .reload_step = 1.0f / (32.0f),
    },
    /*{
        .print_name = "Rocket Launcher",
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
    },*/
    {
        .print_name = "Hyperblaster",
        .classname = "weapon_hyperblaster",

        .ammo_classname = "ammo_cells",
        .pickup_drop_ammo = 50, // "50 Per Battery" [*1]
        .fire_ammo = 1,

        .model_name = "models/weapons/v_hyperb/tris.md2",
        .idle_frame = 21,
        .take_frames = {1, 2, 4, 5},

        .cook_step = 0.0f,
        .fire_delay = 0, // "10 discharges (cells) per second" [*1]
        .muzzle_flash = MZ_HYPERBLASTER,
        .trail_effect = NO_TRAIL,

        //.damage = 20, // "10 single, 20 deathmatch" [*1], source is wrong
        // is 20 in single, 15 in deathmatch [*2]

        .damage = 14, // 14 feels fine

        .projectiles_no = 1,
        .impact_effect = TE_BLASTER,

        .recoil_step = (1.0f) / 15.0f, // 15 shots
        //.recoil_restore_step = 0.11f, // Fine, a bit slow
        .recoil_restore_step = 0.16f, // Something similar to the Smg
        .spread = 10.0f,
        .spread_crouch_scale = 0.4f,
        .view_recoil_scale = 4.0f,
        .view_shake_scale = 1.4f,

        .magazine_size = 25,
        .reload_sound_name = "weapons/hyprbre.wav",
        .reload_step = 1.0f / (36.0f),
    },
    {
        .print_name = "Railgun",
        .classname = "weapon_railgun",

        .ammo_classname = "ammo_slugs",
        .pickup_drop_ammo = 10,
        .fire_ammo = 1,

        .model_name = "models/weapons/v_rail/tris.md2",
        .idle_frame = 19,
        .take_frames = {0, 1, 2, 3},

        .cook_step = 0.0f,
        //.fire_delay = 15 - 1, // "One shot (slug) every 1.5 seconds" [*1]
        .fire_delay = 0,
        .muzzle_flash = MZ_RAILGUN,

        .damage = 150,
        .projectiles_no = 1,
        .impact_effect = TE_HEATBEAM_SPARKS,
        .trail_effect = TE_RAILTRAIL,

        .recoil_step = 1.0f,
        .recoil_restore_step = 0.1f, // [*2]
        .spread = 0.0f,
        .spread_crouch_scale = 0.0f,
        .view_recoil_scale = 9.0f,
        .view_shake_scale = 6.0f,

        .magazine_size = 1,
        .reload_sound_name = "weapons/railgre.wav",
        .reload_step = 1.0f / (1.7f * 10.0f), // [*2]
    },
    /*{
        .print_name = "BFG10K",
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
    },*/
    {
        .print_name = "Hand Grenade",
        .classname = "ammo_grenades",

        .ammo_classname = "ammo_grenades", // Is its own ammo
        .pickup_drop_ammo = 5,
        .fire_ammo = 1,

        .model_name = "models/weapons/v_handgr/tris.md2",
        .idle_frame = 33,
        .take_frames = {33, 33, 33, 33},

        .cook_step = 1.0f / (5.0f * 10.0f),
        //.fire_delay = 20 - 1, // "About one every two seconds" [*1]
        .fire_delay = 10 - 1,
        .muzzle_flash = MZ_ROCKET,
        .trail_effect = NO_TRAIL,

        .damage = 50,
        .projectiles_no = 1,
        .impact_effect = TE_SHOTGUN,
        .projectiles_spray = 0.0f,

        .recoil_step = 0.0f,
        .recoil_restore_step = 0.0f,
        .spread = 0.0f,
        .spread_crouch_scale = 0.0f,
        .view_recoil_scale = 0.0f,
        .view_shake_scale = 0.0f,

        .magazine_size = 0,
        .reload_sound_name = NULL,
    },
};


// ============================================


static int sMin(int a, int b)
{
	return (a < b) ? a : b;
}

static int sMax(int a, int b)
{
	return (a > b) ? a : b;
}

static float sEasing(float x, float k)
{
	// Normalised Tunable Sigmoid Function (V2)
	// https://dhemery.github.io/DHE-Modules/technical/sigmoid/

	// (x - k * x) / (k - 2.0f * k * fabsf(x) + 1.0f);

	// We don't need the abs
	return (x - k * x) / (k - 2.0f * k * x + 1.0f);
}

static const struct koiWeaponBehaviour* sFindBehaviour(const char* classname)
{
	const struct koiWeaponBehaviour* b = NULL;

	for (int i = 0; i < KOI_WEAPONS_NO; i += 1)
	{
		if (strcmp(classname, BEHAVIOURS[i].classname) == 0)
		{
			b = BEHAVIOURS + i;
			break;
		}
	}

	return b;
}

static size_t sBehaviourIndex(const struct koiWeaponBehaviour* b)
{
	return ((size_t)(b) - (size_t)(BEHAVIOURS)) / sizeof(struct koiWeaponBehaviour);
}

static const struct koiWeaponBehaviour* sBehaviourFromIndex(size_t i)
{
	return BEHAVIOURS + i;
}

static void sPlaySoundAndWait(struct edict_s* player, const char* filename, float wait)
{
	struct koiWeaponState* state = &player->client->weapon;

	if (state->sound_wait > level.time)
		return;

	if (filename != NULL)
		gi.sound(player, 8, gi.soundindex(filename), 1, ATTN_NORM, 0);

	state->sound_wait = level.time + wait;
}

static void sPlaySound(struct edict_s* player, const char* filename)
{
	if (filename != NULL)
		gi.sound(player, 7, gi.soundindex(filename), 1, ATTN_NORM, 0);
}


void PlayerNoise(struct edict_s* who, vec3_t where, int type)
{
	(void)who;
	(void)where;
	(void)type;
}


qboolean koiWeaponPickup(struct edict_s* weapon_item_ent, struct edict_s* player_ent)
{
	// Use item classname to find an appropiate behaviour,
	// this checks if is an item of a weapon under our control
	const struct koiWeaponBehaviour* b = sFindBehaviour(weapon_item_ent->item->classname);

	if (b == NULL)
	{
		gi.cprintf(player_ent, PRINT_HIGH, "koiWeaponPickup(): item '%s' is not a weapon!\n",
		           weapon_item_ent->item->classname);
		goto return_failure;
	}

	gi.cprintf(player_ent, PRINT_HIGH, "koiWeaponPickup(): '%s', index: %i\n", b->classname,
	           ITEM_INDEX(weapon_item_ent->item));

	// Mark player persistent inventory, outside code require this variable.
	// Funny little detail, weapons accumulate in Quake 2, it's possible to carry
	// more than one, also possible to drop them
	player_ent->client->pers.inventory[ITEM_INDEX(weapon_item_ent->item)] += 1;

	// Add some ammo
	if (strcmp(b->ammo_classname, b->classname) == 0)
	{
		// Weapon is its own ammo, so we increase
		// how much weapons of this kind we carry

		if ((weapon_item_ent->spawnflags & DROPPED_ITEM) != 0)
			player_ent->client->pers.inventory[ITEM_INDEX(weapon_item_ent->item)] += weapon_item_ent->count;
		else
			player_ent->client->pers.inventory[ITEM_INDEX(weapon_item_ent->item)] += (int)(b->pickup_drop_ammo) - 1;
	}
	else if (b->ammo_classname != NULL)
	{
		// Normal weapon, we increase the magazine
		// and the ammo inventory

		struct gitem_s* ammo_item = FindItemByClassname(b->ammo_classname);
		int* magazine = &player_ent->client->pers.magazines[sBehaviourIndex(b)];
		int ammo_to_add = (int)(b->pickup_drop_ammo);

		if ((weapon_item_ent->spawnflags & DROPPED_ITEM) != 0)
		{
			if (weapon_item_ent->count == 0)
				gi.cprintf(player_ent, PRINT_HIGH, "Weapon without ammo\n");
			ammo_to_add = weapon_item_ent->count;
		}

		if (*magazine == 0)
		{
			*magazine += ammo_to_add;

			// Remainder goes to inventory
			if (*magazine > (int)(b->magazine_size))
			{
				Add_Ammo(player_ent, ammo_item, *magazine - (int)(b->magazine_size));
				*magazine = (int)(b->magazine_size);
			}

			// Since we refilled the magazine of current active weapon, magically,
			// lets go back to take state to sell the idea that our character took
			// the new weapon instead
			if (player_ent->client->weapon.behaviour_index == sBehaviourIndex(b))
			{
				player_ent->client->weapon.frame = 0;
				player_ent->client->weapon.stage = KOI_WEAPON_TAKE;
			}
		}
		else
		{
			Add_Ammo(player_ent, ammo_item, (int)(ammo_to_add));
		}
	}

	// Change weapon
	koiWeaponUse(player_ent, weapon_item_ent->item);

	// Bye!
	return true; // Weapon taken

return_failure:
	return false; // Weapon not taken
}


void koiWeaponUse(struct edict_s* player, struct gitem_s* weapon_item)
{
	struct koiWeaponState* state = &player->client->weapon;
	const struct koiWeaponBehaviour* prev_b = sBehaviourFromIndex(state->behaviour_index);
	const struct koiWeaponBehaviour* b = sFindBehaviour(weapon_item->classname);

	if (b == NULL)
	{
		gi.cprintf(player, PRINT_HIGH, "koiWeaponUse(): item '%s' is not a weapon!\n", weapon_item->classname);
		return;
	}

	gi.cprintf(player, PRINT_HIGH, "koiWeaponUse(): '%s', index: %i\n", b->classname, ITEM_INDEX(weapon_item));

	if (prev_b == b)
		return;

	// Honor this cvar, don't use an empty weapon
	if (g_select_empty->value == 0 && b->ammo_classname != NULL)
	{
		const struct gitem_s* ammo_item = FindItemByClassname(b->ammo_classname);
		const int available_ammo = player->client->pers.inventory[ITEM_INDEX(ammo_item)] +
		                           (int)(player->client->pers.magazines[sBehaviourIndex(b)]);

		if (available_ammo < (int)(b->fire_ammo))
			return;
	}

	// Mark player persistent weapon, outside code require this variables
	player->client->pers.lastweapon = player->client->pers.weapon;
	player->client->pers.weapon = weapon_item;

	// Set state
	{
		state->stage = KOI_WEAPON_TAKE;
		state->restore_recoil = 1;

		if (b->ammo_classname != NULL)
			state->ammo_item_index = ITEM_INDEX(FindItemByClassname(b->ammo_classname));
		else
			state->ammo_item_index = 0;

		state->behaviour_index = sBehaviourIndex(b);
		// state->recoil = 1.0f; // Penalize change weapons
		state->recoil = 0.0f; // TODO: every weapon should have its own recoil value
		state->frame = 0;
		state->fire_wait = 0;
		state->cook_progress = 0.0f;

		player->client->ps.stats[27] = 0;
		player->client->ps.stats[28] = 0;
		player->client->ps.stats[29] = 0;
		player->client->ps.stats[30] = 0;
		player->client->ps.stats[31] = 0;

		sPlaySound(player, "weapons/change.wav");
	}
}


void koiWeaponDrop(struct edict_s* player, struct gitem_s* weapon_item)
{
	const struct koiWeaponBehaviour* b = sFindBehaviour(weapon_item->classname);

	if (b == NULL)
	{
		gi.cprintf(player, PRINT_HIGH, "koiWeaponDrop(): item '%s' is not a weapon!\n", weapon_item->classname);
		return;
	}

	gi.cprintf(player, PRINT_HIGH, "koiWeaponDrop(): '%s', index: %i\n", b->classname, ITEM_INDEX(weapon_item));

	if (player->client->pers.inventory[ITEM_INDEX(weapon_item)] <= 0)
	{
		// We already drop all weapons of this kind
		gi.cprintf(player, PRINT_HIGH, "koiWeaponDrop(): no weapon '%s' in inventory\n", b->classname);
		return;
	}

	// Mark persistent inventory, for the weapon and ammo
	size_t ammo_index = 0;
	int ammo_to_drop = 0;

	if (strcmp(b->ammo_classname, b->classname) != 0)
	{
		// Ammo, just the magazine, even if empty
		ammo_to_drop = player->client->pers.magazines[sBehaviourIndex(b)];
		player->client->pers.magazines[sBehaviourIndex(b)] = 0;

		// Weapon
		player->client->pers.inventory[ITEM_INDEX(weapon_item)] -= 1;
	}
	else
	{
		// Case where weapon is its own ammo (notice the '-1's)
		ammo_index = ITEM_INDEX(weapon_item);

		if (player->client->pers.inventory[ammo_index] > (int)(b->pickup_drop_ammo))
		{
			player->client->pers.inventory[ammo_index] -= (int)(b->pickup_drop_ammo);
			ammo_to_drop = (int)(b->pickup_drop_ammo) - 1;
		}
		else
		{
			player->client->pers.inventory[ammo_index] -= 1;
			ammo_to_drop = 0;
		}
	}

	// Drop weapon (spawns an item)
	struct edict_s* dropped_item = Drop_Item(player, weapon_item);
	dropped_item->spawnflags = (DROPPED_ITEM | DROPPED_PLAYER_ITEM);
	dropped_item->count = ammo_to_drop;

	// Should we change current weapon?
	if (
	    // a. We where using it and now we have zero weapons of this kind left
	    (player->client->pers.weapon == weapon_item && player->client->pers.inventory[ITEM_INDEX(weapon_item)] == 0) ||

	    // b. We don't have ammo for it
	    (ammo_index > 0 && player->client->pers.inventory[ammo_index] == 0))
	{
		koiWeaponUse(player, FindItemByClassname("weapon_blaster"));
	}
	else
	{
		// Sell the idea that character took a new weapon of the same kind,
		// also some weapons stages have the assumption of ammo being present,
		// which we modified in procedures above
		// TODO, repeated code
		player->client->weapon.frame = 0;
		player->client->weapon.stage = KOI_WEAPON_TAKE;
	}
}


void koiWeaponDev(const struct edict_s* player) {}


// ============================================


static int sTraceRay(const struct edict_s* player, vec3_t start, vec3_t direction, trace_t* out)
{
	vec3_t end;
	VectorMA(start, 8192, direction, end);

	*out = gi.trace(start, NULL, NULL, end, player, MASK_SHOT);
	return 0;

	if (out->allsolid == true) // An invalid trace, player is inside a solid
		return 1;
}

static void sHitscan(const struct edict_s* player, const struct koiWeaponBehaviour* b)
{
	struct koiWeaponState* state = &player->client->weapon;
	int knockback = 10;
	int means_of_death = MOD_UNKNOWN;

	// Calculate spread
	float spread;
	{
		spread = b->spread;
		if ((player->client->ps.pmove.pm_flags & PMF_DUCKED) != 0)
			spread *= b->spread_crouch_scale;
	}

	// Calculate ray direction
	vec3_t direction;
	vec3_t direction_forward;
	{
		const float q = frandk() * 2.0f * (float)(M_PI);               // Polar to avoid a square spread
		const float r = powf(frandk(), 2.0f) * spread * state->recoil; // Bias towards centre
		const float random_x = cosf(q) * r;
		const float random_y = sinf(q) * r;

		// Maths
		VectorSet(direction,                             //
		          player->client->v_angle[0] + random_y, //
		          player->client->v_angle[1] + random_x, //
		          player->client->v_angle[2]);

		AngleVectors(direction, direction_forward, NULL, NULL);
	}

	// Start origin
	vec3_t start;
	{
		vec3_t dir;
		vec3_t forward;
		vec3_t side;

		VectorSet(dir,                        //
		          player->client->v_angle[0], //
		          player->client->v_angle[1], //
		          player->client->v_angle[2]);

		AngleVectors(dir, forward, side, NULL);
		VectorMA(player->s.origin, 10.0f, forward, start);
		VectorMA(start, (player->client->pers.hand == LEFT_HANDED) ? -4.0f : 4.0f, side, start);
		start[2] += (float)(player->viewheight) - 4.0f;
	}

	// Muzzle flash
	if (b->muzzle_flash != NO_MUZZLE_FLASH)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(player - g_edicts);
		gi.WriteByte((int)(b->muzzle_flash));
		gi.multicast(start, MULTICAST_PVS);
	}

	// Trace a ray
	trace_t tr;

	for (int i = 0; i < b->projectiles_no; i += 1)
	{
		if (sTraceRay(player, start, direction_forward, &tr) != 0)
			return;

		// Impact puff
		if ((tr.surface->flags & SURF_SKY) == 0)
		{
			if (tr.ent->takedamage)
			{
				T_Damage(tr.ent, player, player, direction_forward, tr.endpos, tr.plane.normal, (int)(b->damage),
				         knockback, DAMAGE_BULLET, means_of_death);
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

		// Trail
		if (b->trail_effect != NO_TRAIL)
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte((int)(b->trail_effect));
			gi.WritePosition(start);
			gi.WritePosition(tr.endpos);
			gi.multicast(tr.endpos, MULTICAST_PVS);
		}

		// Update direction, no fancy polar here
		direction_forward[0] = direction[0] + (frandk() - 0.5f) * b->projectiles_spray;
		direction_forward[1] = direction[1] + (frandk() - 0.5f) * b->projectiles_spray;

		AngleVectors(direction_forward, direction_forward, NULL, NULL);
	}
}

static void sTakeStage(struct edict_s* player)
{
	struct koiWeaponState* state = &player->client->weapon;
	const struct koiWeaponBehaviour* b = sBehaviourFromIndex(state->behaviour_index);

	// Animation frames
	player->client->ps.gunframe = (int)(b->take_frames[state->frame]);

	// We need to trick client's model interpolation
	if (state->frame == 1)
		player->client->ps.gunindex = gi.modelindex(b->model_name);

	// Animation ends
	if (state->frame >= KOI_TAKE_FRAMES_NO)
	{
		player->client->ps.gunframe = (int)(b->idle_frame);
		state->stage = KOI_WEAPON_IDLE; // Change stage!
	}
}

static void sAddRecoil(struct koiWeaponState* state, const struct koiWeaponBehaviour* b)
{
	state->recoil += b->recoil_step;
	if (state->recoil > 1.0f)
		state->recoil = 1.0f;

	state->view_recoil = sEasing(state->recoil, -0.60f) * b->view_recoil_scale;
	state->view_shake = b->view_shake_scale;
}

static void sRestoreRecoil(struct koiWeaponState* state, const struct koiWeaponBehaviour* b)
{
	state->recoil -= b->recoil_restore_step;
	if (state->recoil < 0.0f)
		state->recoil = 0.0f;

	state->view_recoil = sEasing(state->recoil, -0.60f) * b->view_recoil_scale;
	state->view_shake = 0;
}

static void sIdleStage(struct edict_s* player)
{
	struct koiWeaponState* state = &player->client->weapon;
	const struct koiWeaponBehaviour* b = sBehaviourFromIndex(state->behaviour_index);

	// Should we reload?
	if (b->ammo_classname != NULL &&
	    strcmp(b->classname, b->ammo_classname) != 0 && // Not weapons that are its own ammo
	    b->fire_ammo != 0 && player->client->pers.magazines[state->behaviour_index] < b->fire_ammo)
	{
		state->stage = KOI_WEAPON_RELOAD; // Change stage!
		state->reload_progress = 0.0f;
	}

	// Should we fire?
	else if ((player->client->buttons & BUTTON_ATTACK) == true)
	{
		state->stage = KOI_WEAPON_FIRE; // Change stage!
	}
}

static void sFireStage(struct edict_s* player)
{
	struct koiWeaponState* state = &player->client->weapon;
	const struct koiWeaponBehaviour* b = sBehaviourFromIndex(state->behaviour_index);

	state->restore_recoil = 1; // In case of an early return,

	// From where subtract ammo, depending on the
	// time of weapon (grenades are its own ammo)
	int* ammo = &player->client->pers.magazines[state->behaviour_index];

	if (b->ammo_classname != NULL && strcmp(b->classname, b->ammo_classname) == 0)
		ammo = &player->client->pers.inventory[state->ammo_item_index];

	// Reach this stage without ammo is a sin
	assert(*ammo >= (int)(b->fire_ammo));

	// Conditions to meet before fire, depending on
	// on a lot of factors and order of execution
	{
		// Wait a bit between shots
		if (state->fire_wait >= state->general_frame)
			return;

		if (b->cook_step == 0.0f)
		{
			// Should we fire?
			if ((player->client->buttons & BUTTON_ATTACK) == false)
				return;
		}
		else
		{
			// Should we fire?, except that this time we ensure
			// that the mouse was released on the wait after firing
			if (state->cook_progress == 0.0f && (player->client->buttons & BUTTON_ATTACK) == false)
				return;

			// Cook
			state->cook_progress += b->cook_step;

			// Don't proceed with firing until player releases the
			// mouse or we burn our food
			if (state->cook_progress <= 1.0f && (player->client->buttons & BUTTON_ATTACK) == true)
				return;

			// Next state after firing
			state->cook_progress = 0.0f;
			state->stage = KOI_WEAPON_IDLE; // An assumption
		}
	}

	// Fire,
	{
		// Process projectile before recoil,
		// so first shot always goes to centre
		sHitscan(player, b);

		// Developers, developers, developers,
		// before recoil as well
		player->client->ps.stats[30] = (short)(state->recoil * 100.0f);

		// Update thingies
		sAddRecoil(state, b);
		state->restore_recoil = 0;
		state->fire_wait = state->general_frame + (unsigned)(b->fire_delay);
	}

	// Subtract ammo
	*ammo -= (int)(b->fire_ammo);

	// We have ammo?
	if (b->fire_ammo != 0 && *ammo < (int)(b->fire_ammo))
	{
		state->restore_recoil = 1;
		state->stage = KOI_WEAPON_RELOAD; // Change stage!
		state->reload_progress = 0.0f;
	}
}

static void sReloadStage(struct edict_s* player)
{
	struct koiWeaponState* state = &player->client->weapon;
	const struct koiWeaponBehaviour* b = sBehaviourFromIndex(state->behaviour_index);

	const int available_ammo =
	    player->client->pers.inventory[state->ammo_item_index] + player->client->pers.magazines[state->behaviour_index];

	// No ammo
	if (available_ammo < (int)(b->fire_ammo) && ((int)(dmflags->value) & DF_INFINITE_AMMO) == 0)
	{
		// Change weapon if current weapon is its own ammo
		if (strcmp(b->ammo_classname, b->classname) == 0)
		{
			koiWeaponUse(player, FindItemByClassname("weapon_blaster"));
			return;
		}

		// Change weapon if player doesn't want to iterate over empty weapons
		else if (g_select_empty->value == 0)
		{
			if (b->fire_ammo == 0)
				gi.cprintf(player, PRINT_HIGH, "No ammo for '%s'\n", b->print_name);
			else
				gi.cprintf(player, PRINT_HIGH, "No enough ammo for '%s'\n", b->print_name);

			koiWeaponUse(player, FindItemByClassname("weapon_blaster"));
			return;
		}

		// If nothing above, just returns
		return;
	}

	// Reload
	if (state->reload_progress == 0.0f)
	{
		// sPlaySoundAndWait(player, "weapons/noammo.wav", 1.0f);
		sPlaySound(player, b->reload_sound_name);
	}

	state->reload_progress += b->reload_step;

	if (state->reload_progress >= 1.0f)
	{
		// Change stage!
		state->stage = KOI_WEAPON_IDLE;

		// Ammo, add to magazine, subtract from ammo
		if (((int)(dmflags->value) & DF_INFINITE_AMMO) == 0)
		{
			const int ammo =
			    sMin(available_ammo, (int)(b->magazine_size)) - player->client->pers.magazines[state->behaviour_index];

			player->client->pers.magazines[state->behaviour_index] += ammo;
			player->client->pers.inventory[state->ammo_item_index] -= ammo;
		}
		else
		{
			player->client->pers.magazines[state->behaviour_index] = (int)(b->magazine_size);
		}
	}
}

void koiWeaponThink(struct edict_s* player)
{
	struct koiWeaponState* state = &player->client->weapon;

	// Retrieve behaviour
	const struct koiWeaponBehaviour* b = sBehaviourFromIndex(state->behaviour_index);

	// Do according the stage
	while (1)
	{
		const enum koiWeaponStage prev_stage = state->stage;

		switch (state->stage)
		{
		case KOI_WEAPON_TAKE: sTakeStage(player); break;
		case KOI_WEAPON_IDLE: sIdleStage(player); break;
		case KOI_WEAPON_FIRE: sFireStage(player); break;
		case KOI_WEAPON_RELOAD: sReloadStage(player); break;
		}

		// Restore recoir, regardless of stage
		if (state->restore_recoil == 1)
			sRestoreRecoil(state, b);

		// If the stage changed, we execute it right now
		// (it's a game running at 10 fps)
		if (state->stage == prev_stage)
			break;
	}

	// Update state
	state->frame += 1;
	state->general_frame += 1;

	// Developers, developers, developers
	{
		player->client->ps.stats[27] = (short)(player->client->pers.magazines[state->behaviour_index]);

		if (b->fire_ammo != 0)
			player->client->ps.stats[28] =
			    (short)(player->client->pers.inventory[ITEM_INDEX(FindItemByClassname(b->ammo_classname))]);
		else
			player->client->ps.stats[28] = (short)(0);

		player->client->ps.stats[29] = (short)(state->cook_progress * 100.0f);
		player->client->ps.stats[31] = (short)(state->recoil * 100.0f);
	}
}
