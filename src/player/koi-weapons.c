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
//  - [    ] deathmatch (changes weapons damage)
//  - [done] g_select_empty
//  - [    ] g_swap_speed (seems to be a Yamagi addition)

// And finally, if there is Quad damage and Silencer, powerups


#define NO_MUZZLE_FLASH 255

static struct koiWeaponBehaviour BEHAVIOURS[KOI_WEAPONS_NO] = {
    {
        .print_name = "Blaster",
        .classname = "weapon_blaster",
        .fire_ammo = 0,

        .model_name = "models/weapons/v_blast/tris.md2",
        .idle_frame = 9,
        .take_frames = {0, 1, 2, 3},

        //.fire_delay = 5 - 1, // "Two shots per second" [*1]
        .fire_delay = 3 - 1, // "Two shots per second" [*1]
        .muzzle_flash = MZ_BLASTER,

        .damage = 10, // "10 in single play, 15 in deathmatch" [*1]
        .projectiles_no = 1,
        .impact_effect = TE_BLASTER,

        .recoil_step = 1.0f,
        .recoil_restore_step = 1.0f / (5.0f - 1.0f), // Match old fire delay, newer is faster
        .spread = 7.0f,
        .spread_crouch_scale = 0.1f, // Almost no spread
        .view_recoil_scale = 4.0f,
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

        //.fire_delay = 10 - 1, // "One discharge (1 shell) per second" [*1]
        .fire_delay = 9 - 1, // A bit faster
        .muzzle_flash = MZ_SHOTGUN,

        .damage = 4, // "4 per pellet, 12 pellets" [*1]
        .projectiles_no = 12,
        .impact_effect = TE_SHOTGUN,
        .projectiles_spray = 10.0f,

        .recoil_step = 1.0f,
        .recoil_restore_step = 1.0f / (10.0f - 1.0f), // Match old fire delay, newer is faster
        .spread = 50.0f,                              // Up to 100 is tolerable
        .spread_crouch_scale = 0.2f,
        .view_recoil_scale = 5.0f,

        .reload_sound_name = "weapons/Shotgre.wav",
        .reload_step = 1.0f / (44.0f),
        .magazine_size = 10,
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

        .fire_delay = 0, // "10 bullets per second" [*1]
        .muzzle_flash = MZ_MACHINEGUN,

        .damage = 8, // "8 per bullet" [*1]
        .projectiles_no = 1,
        .impact_effect = TE_GUNSHOT,

        .recoil_step = (1.0f) / 15.0f, // 15 shots
        .recoil_restore_step = 0.11f,
        .spread = 10.0f,
        .spread_crouch_scale = 0.4f,
        .view_recoil_scale = 7.0f,

        .reload_sound_name = "weapons/machgre.wav",
        .reload_step = 1.0f / (32.0f),
        .magazine_size = 50,
    },
    {
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
    },
    {
        .print_name = "Hyperblaster",
        .classname = "weapon_hyperblaster",

        .ammo_classname = "ammo_cells",
        .pickup_drop_ammo = 50, // "50 Per Battery" [*1]
        .fire_ammo = 1,

        .model_name = "models/weapons/v_hyperb/tris.md2",
        .idle_frame = 21,
        .take_frames = {1, 2, 4, 5},

        .fire_delay = 0, // "10 discharges (cells) per second" [*1]
        .muzzle_flash = MZ_HYPERBLASTER,

        .damage = 10, // "10 single, 20 deathmatch" [*1]
        .projectiles_no = 1,
        .impact_effect = TE_BLASTER,

        .recoil_step = (1.0f) / 12.0f, // 12 shots
        .recoil_restore_step = 0.09f,
        .spread = 12.0f,
        .spread_crouch_scale = 0.4f,
        .view_recoil_scale = 2.5f,

        .reload_sound_name = "weapons/hyprbre.wav",
        .reload_step = 1.0f / (36.0f),
        .magazine_size = 50,
    },
    {
        .print_name = "Railgun",
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
    },
    {
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
    },
    {
        .print_name = "Hand Grenade",
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
    },
};


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

void PlayerNoise(struct edict_s* who, vec3_t where, int type) {}


// ============================================


static void sPlaySoundAndWait(struct edict_s* player, const char* filename, float wait)
{
	if (player->client->weapon.sound_wait > level.time)
		return;

	gi.sound(player, CHAN_AUTO, gi.soundindex(filename), 1, ATTN_NORM, 0);
	player->client->weapon.sound_wait = level.time + wait;
}
static void sPlaySound(struct edict_s* player, const char* filename)
{
	gi.sound(player, 7, gi.soundindex(filename), 1, ATTN_NORM, 0);
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
		// Weapon is its own ammo, so instead we increase
		// how much weapons of this kind we carry
		if ((weapon_item_ent->spawnflags & ITEM_WEAPON_WITH_NO_AMMO) == 0)
			player_ent->client->pers.inventory[ITEM_INDEX(weapon_item_ent->item)] += ((int)(b->pickup_drop_ammo) - 1);
	}
	else if (b->ammo_classname != NULL)
	{
		// Ask inventory for what kind of ammo we must add
		if ((weapon_item_ent->spawnflags & ITEM_WEAPON_WITH_NO_AMMO) == 0)
		{
			struct gitem_s* ammo_item = FindItemByClassname(b->ammo_classname);
			Add_Ammo(player_ent, ammo_item, (int)(b->pickup_drop_ammo));
		}
		else // Oh no! a weapon with no ammo
			gi.cprintf(player_ent, PRINT_HIGH, "Weapon without ammo\n");
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
	const struct koiWeaponBehaviour* prev_b = sBehaviourFromIndex(player->client->weapon.current_behaviour_index);
	const struct koiWeaponBehaviour* b = sFindBehaviour(weapon_item->classname);

	if (b == NULL)
	{
		gi.cprintf(player, PRINT_HIGH, "koiWeaponUse(): item '%s' is not a weapon!\n", weapon_item->classname);
		return;
	}

	gi.cprintf(player, PRINT_HIGH, "koiWeaponUse(): '%s', index: %i\n", b->classname, ITEM_INDEX(weapon_item));

	if (prev_b == b)
		return;

	if (g_select_empty->value == 0)
	{
		const struct gitem_s* ammo_item = FindItemByClassname(b->ammo_classname);
		if (ammo_item != NULL && player->client->pers.inventory[ITEM_INDEX(ammo_item)] < b->fire_ammo)
			return;
	}

	// Mark player persistent weapon, outside code require this variables
	player->client->pers.lastweapon = player->client->pers.weapon;
	player->client->pers.weapon = weapon_item;

	// Set state
	{
		player->client->weapon.stage = KOI_WEAPON_TAKE;
		player->client->weapon.fired = 0;

		if (b->ammo_classname != NULL)
			player->client->weapon.current_ammo_item_index = ITEM_INDEX(FindItemByClassname(b->ammo_classname));
		else
			player->client->weapon.current_ammo_item_index = 0;

		player->client->weapon.current_behaviour_index = sBehaviourIndex(b);
		// player->client->weapon.recoil = 1.0f; // Penalize change weapons
		player->client->weapon.recoil = 0.0f; // TODO: every weapon should have its own recoil value
		player->client->weapon.frame = 0;
		player->client->weapon.fire_wait = 0;

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

	// Mark player persistent inventory
	// - For ammo, we subtract some, if possible, if not we drop a weapon tagged with 'no ammo'
	// - For weapon, we subtract one
	int tag_with = 0;
	int ammo_index = 0;

	if (strcmp(b->ammo_classname, b->classname) != 0)
	{
		// Ammo
		if (b->fire_ammo != 0)
		{
			struct gitem_s* ammo_item = FindItemByClassname(b->ammo_classname);
			ammo_index = ITEM_INDEX(ammo_item);

			if (player->client->pers.inventory[ammo_index] >= (int)(b->pickup_drop_ammo))
				player->client->pers.inventory[ammo_index] -= (int)(b->pickup_drop_ammo);
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
			player->client->pers.inventory[ammo_index] -= (int)(b->pickup_drop_ammo);
		else
		{
			player->client->pers.inventory[ammo_index] -= 1;
			tag_with = ITEM_WEAPON_WITH_NO_AMMO;
		}
	}

	// Drop weapon (spawns an item)
	struct edict_s* dropped_item = Drop_Item(player, weapon_item);
	dropped_item->spawnflags |= (tag_with | DROPPED_ITEM | DROPPED_PLAYER_ITEM); // Tags used in Dm to respawn items

	// Should we change current weapon?
	if (
	    // We where using it and now we have zero weapons of this kind left
	    (player->client->pers.weapon == weapon_item && player->client->pers.inventory[ITEM_INDEX(weapon_item)] == 0) ||
	    // We don't have any ammo for it
	    (ammo_index > 0 && player->client->pers.inventory[ammo_index] == 0))
	{
		koiWeaponUse(player, FindItemByClassname("weapon_blaster"));
	}
}

void koiWeaponDev(const struct edict_s* player) {}


// ============================================


static inline float sEasing(float x, float e)
{
	const float epsilon = 1.0F / 1024.0F;
	return x = (x) / (x + e * (1.0F - x) + epsilon);
}

static int sTraceRay(const struct edict_s* player, const struct koiWeaponBehaviour* b, vec3_t direction, trace_t* out)
{
	vec3_t start;
	VectorSet(start, player->s.origin[0], player->s.origin[1], player->s.origin[2] + player->viewheight);

	vec3_t end;
	VectorMA(start, 8192, direction, end);

	*out = gi.trace(start, NULL, NULL, end, player, MASK_SHOT);
	return 0;

	if (out->allsolid == true) // An invalid trace, player is inside a solid
		return 1;
}

static void sHitscan(const struct edict_s* player, const struct koiWeaponBehaviour* b)
{
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
		const float recoil = player->client->weapon.recoil;

		const float q = frandk() * M_PI * 2.0f;                 // Polar to avoid a square spread
		const float r = powf(frandk(), 2.0f) * spread * recoil; // Bias towards centre
		const float random_x = cos(q) * r;
		const float random_y = sin(q) * r;

		// Maths
		VectorSet(direction,                             //
		          player->client->v_angle[0] + random_y, //
		          player->client->v_angle[1] + random_x, //
		          player->client->v_angle[2]);

		AngleVectors(direction, direction_forward, NULL, NULL);
	}

	// Trace a ray
	trace_t tr;

	for (int i = 0; i < b->projectiles_no; i += 1)
	{
		if (sTraceRay(player, b, direction_forward, &tr) != 0)
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

		// Update direction, no fancy polar here
		direction_forward[0] = direction[0] + (frandk() - 0.5f) * b->projectiles_spray;
		direction_forward[1] = direction[1] + (frandk() - 0.5f) * b->projectiles_spray;

		AngleVectors(direction_forward, direction_forward, NULL, NULL);
	}
}

static void sTakeStage(struct edict_s* player)
{
	const struct koiWeaponBehaviour* b = sBehaviourFromIndex(player->client->weapon.current_behaviour_index);

	// We need to trick client's model interpolation
	if (player->client->weapon.frame < KOI_TAKE_FRAMES_NO)
		player->client->ps.gunframe = (int)(b->take_frames[player->client->weapon.frame]);
	if (player->client->weapon.frame == 1)
		player->client->ps.gunindex = gi.modelindex(b->model_name);

	// Animation ends
	if (player->client->weapon.frame == KOI_TAKE_FRAMES_NO) // Change stage!
	{
		player->client->ps.gunframe = (int)(b->idle_frame);
		player->client->weapon.stage = KOI_WEAPON_IDLE;
	}
}

static void sIdleStage(struct edict_s* player)
{
	// Should we fire?
	if ((player->client->buttons & BUTTON_ATTACK) == true) // Change stage!
		player->client->weapon.stage = KOI_WEAPON_FIRE;
}

static void sFireStage(struct edict_s* player)
{
	const struct koiWeaponBehaviour* b = sBehaviourFromIndex(player->client->weapon.current_behaviour_index);

	player->client->weapon.fired = 0; // Assume it in case of an early return

	// Should we be firing?
	if ((player->client->buttons & BUTTON_ATTACK) == false) // Change stage!
	{
		player->client->weapon.stage = KOI_WEAPON_IDLE;
		return;
	}

	// Can we fire?
	if (player->client->weapon.fire_wait >= player->client->weapon.general_frame)
		return;

	// Ammo managment
	if (b->fire_ammo != 0)
	{
		// No ammo
		if (player->client->pers.magazines[player->client->weapon.current_behaviour_index] <
		    (int)(b->fire_ammo)) // Change stage!
			goto no_ammo;

		// Subtract ammo
		if (((int)(dmflags->value) & DF_INFINITE_AMMO) == 0)
			player->client->pers.magazines[player->client->weapon.current_behaviour_index] -= (int)(b->fire_ammo);
	}

	// Fire,
	// before recoil so first shot always goes to centre
	player->client->weapon.fire_wait = player->client->weapon.general_frame + (unsigned)(b->fire_delay);
	sHitscan(player, b);

	// Developers, developers, developers
	{
		player->client->ps.stats[30] = (short)(player->client->weapon.recoil * 100.0f);
	}

	// Apply recoil
	player->client->weapon.recoil += b->recoil_step;
	if (player->client->weapon.recoil > 1.0f)
		player->client->weapon.recoil = 1.0f;

	// Client effect
	if (b->muzzle_flash != NO_MUZZLE_FLASH)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(player - g_edicts);
		gi.WriteByte((int)(b->muzzle_flash));
		gi.multicast(player->s.origin, MULTICAST_PVS);
	}

	// No ammo?, change weapon immediatly
	// TODO: yep, a repeated procedure isn't elegant. The tricky part is 'g_select_empty',
	// as above check happens before firing an empty weapon, while this one after firing
	if (b->fire_ammo != 0 && player->client->pers.magazines[player->client->weapon.current_behaviour_index] <
	                             (int)(b->fire_ammo)) // Change stage!
	{
	no_ammo:
		player->client->weapon.stage = KOI_WEAPON_RELOAD;
		player->client->weapon.reload_progress = 0.0f;
		return;
	}

	// Bye!
	player->client->weapon.fired = 1;
}

static inline int sMin(int a, int b)
{
	return (a < b) ? a : b;
}

static void sReloadStage(struct edict_s* player)
{
	const struct koiWeaponBehaviour* b = sBehaviourFromIndex(player->client->weapon.current_behaviour_index);

	// No ammo
	if (player->client->pers.inventory[player->client->weapon.current_ammo_item_index] <= 0)
	{
		sPlaySoundAndWait(player, "weapons/noammo.wav", 1.0f);
		player->client->weapon.stage = KOI_WEAPON_IDLE; // Change stage!
		return;
	}

	// Reload
	if (player->client->weapon.reload_progress == 0.0f)
	{
		sPlaySoundAndWait(player, "weapons/noammo.wav", 1.0f);
		sPlaySound(player, b->reload_sound_name);
	}

	player->client->weapon.reload_progress += b->reload_step;

	if (player->client->weapon.reload_progress >= 1.0f)
	{
		const int ammo = sMin(player->client->pers.inventory[player->client->weapon.current_ammo_item_index],
		                      (int)(b->magazine_size));

		// Add to magazine, subtract from ammo
		player->client->pers.magazines[player->client->weapon.current_behaviour_index] += ammo;
		player->client->pers.inventory[player->client->weapon.current_ammo_item_index] -= ammo;

		// Change stage!
		player->client->weapon.stage = KOI_WEAPON_IDLE;
	}

	// gi.cprintf(player, PRINT_HIGH, "koiWeaponReload(): '%f'\n", player->client->weapon.reload_progress);

	/*if (g_select_empty->value == 0)
	{
	    if (player->client->pers.inventory[player->client->weapon.current_ammo_item_index] == 0)
	        gi.cprintf(player, PRINT_HIGH, "No ammo for '%s'\n", b->print_name);
	    else
	        gi.cprintf(player, PRINT_HIGH, "No enough ammo for '%s'\n", b->print_name);

	    koiWeaponUse(player, FindItemByClassname("weapon_blaster"));
	}*/
}

static void sCommonStage(struct edict_s* player)
{
	const struct koiWeaponBehaviour* b = sBehaviourFromIndex(player->client->weapon.current_behaviour_index);

	// Restore recoil
	if (player->client->weapon.fired == 0)
	{
		player->client->weapon.recoil -= b->recoil_restore_step;
		if (player->client->weapon.recoil < 0.0f)
			player->client->weapon.recoil = 0.0f;
	}
}

void koiWeaponThink(struct edict_s* player)
{
	// Retrieve behaviour
	const struct koiWeaponBehaviour* b = sBehaviourFromIndex(player->client->weapon.current_behaviour_index);

	// Do according the stage
	for (int i = 0; i < 2; i += 1)
	{
		const enum koiWeaponStage prev_stage = player->client->weapon.stage;

		switch (player->client->weapon.stage)
		{
		case KOI_WEAPON_TAKE: sTakeStage(player); break;
		case KOI_WEAPON_IDLE: sIdleStage(player); break;
		case KOI_WEAPON_FIRE: sFireStage(player); break;
		case KOI_WEAPON_RELOAD: sReloadStage(player); break;
		}

		sCommonStage(player);

		// If the stage changed, we execute it right now
		// (it's a game running at 10 fps)
		if (player->client->weapon.stage == prev_stage)
			break;
	}

	// Update state
	player->client->weapon.frame += 1;
	player->client->weapon.general_frame += 1;

	// Developers, developers, developers
	{
		player->client->ps.stats[28] =
		    (short)(player->client->pers.magazines[player->client->weapon.current_behaviour_index]);

		if (b->fire_ammo != 0)
			player->client->ps.stats[29] =
			    (short)(player->client->pers.inventory[ITEM_INDEX(FindItemByClassname(b->ammo_classname))]);
		else
			player->client->ps.stats[29] = (short)(0);

		player->client->ps.stats[31] = (short)(player->client->weapon.recoil * 100.0f);
	}
}
