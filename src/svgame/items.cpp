/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "g_local.h"         // Include SVGame funcs.
#include "utils.h"           // Include Utilities funcs.
#include "player/hud.h"      // Include HUD funcs.

#include "weapons/blaster.h"
#include "weapons/machinegun.h"
#include "weapons/shotgun.h"
#include "weapons/supershotgun.h"

qboolean    Pickup_Weapon(Entity *ent, Entity *other);
void        Use_Weapon(Entity *ent, gitem_t *inv);
void        Drop_Weapon(Entity *ent, gitem_t *inv);

gitem_armor_t bodyarmor_info    = {100, 200, .80f, .60f, ARMOR_BODY};

static int  body_armor_index;
#define HEALTH_IGNORE_MAX   1
#define HEALTH_TIMED        2

//======================================================================

/*
===============
GetItemByIndex
===============
*/
gitem_t *GetItemByIndex(int index)
{
    if (index == 0 || index >= game.numberOfItems)
        return NULL;

    return &itemlist[index];
}


/*
===============
FindItemByClassname

===============
*/
gitem_t *FindItemByClassname(const char *className)
{
    int     i;
    gitem_t *it;

    it = itemlist;
    for (i = 0 ; i < game.numberOfItems ; i++, it++) {
        if (!it->className)
            continue;
        if (!Q_stricmp(it->className, className))
            return it;
    }

    return NULL;
}

/*
===============
FindItem

===============
*/
gitem_t *FindItem(const char *pickup_name) // C++20: STRING: Added const to char*
{
    int     i;
    gitem_t *it;

    it = itemlist;
    for (i = 0 ; i < game.numberOfItems ; i++, it++) {
        if (!it->pickupName)
            continue;
        if (!Q_stricmp(it->pickupName, pickup_name))
            return it;
    }

    return NULL;
}

//======================================================================

void DoRespawn(Entity *ent)
{
    if (!ent)
        return;

    if (ent->team) {
        Entity *master;
        int count;
        int choice;

        master = ent->teamMasterPtr;

        for (count = 0, ent = master; ent; ent = ent->chain, count++)
            ;

        choice = rand() % count;

        for (count = 0, ent = master; count < choice; ent = ent->chain, count++)
            ;
    }

    ent->serverFlags &= ~EntityServerFlags::NoClient;
    ent->solid = Solid::Trigger;
    gi.LinkEntity(ent);

    // send an effect
    ent->state.event = EntityEvent::ItemRespawn;
}

void SetRespawn(Entity *ent, float delay)
{
    if (!ent)
        return;

    ent->flags |= EntityFlags::Respawn;
    ent->serverFlags |= EntityServerFlags::NoClient;
    ent->solid = Solid::Not;
    ent->nextThink = level.time + delay;
    ent->Think = DoRespawn;
    gi.LinkEntity(ent);
}


//======================================================================

qboolean Pickup_Powerup(Entity *ent, Entity *other)
{
    int     quantity;

    quantity = other->client->persistent.inventory[ITEM_INDEX(ent->item)];
    if ((skill->value == 1 && quantity >= 2) || (skill->value >= 2 && quantity >= 1))
        return false;

    if ((coop->value) && (ent->item->flags & IT_STAY_COOP) && (quantity > 0))
        return false;

    other->client->persistent.inventory[ITEM_INDEX(ent->item)]++;

    if (deathmatch->value) {
        if (!(ent->spawnFlags & ItemSpawnFlags::DroppedItem))
            SetRespawn(ent, ent->item->quantity);
    }

    return true;
}

void Drop_General(Entity *ent, gitem_t *item)
{
    Drop_Item(ent, item);
    ent->client->persistent.inventory[ITEM_INDEX(item)]--;
    HUD_ValidateSelectedItem(ent);
}

//======================================================================

qboolean Add_Ammo(Entity *ent, gitem_t *item, int count)
{
    int         index;
    int         max;

    if (!ent)
        return false;

    if (!ent->client)
        return false;

    if (item->tag == AmmoType::Bullets)
        max = ent->client->persistent.maxBullets;
    else if (item->tag == AmmoType::Shells)
        max = ent->client->persistent.maxShells;
    else if (item->tag == AmmoType::Rockets)
        max = ent->client->persistent.maxRockets;
    else if (item->tag == AmmoType::Grenade)
        max = ent->client->persistent.maxGrenades;
    else if (item->tag == AmmoType::Cells)
        max = ent->client->persistent.maxCells;
    else if (item->tag == AmmoType::Slugs)
        max = ent->client->persistent.maxSlugs;
    else
        return false;

    index = ITEM_INDEX(item);

    if (ent->client->persistent.inventory[index] == max)
        return false;

    ent->client->persistent.inventory[index] += count;

    if (ent->client->persistent.inventory[index] > max)
        ent->client->persistent.inventory[index] = max;

    return true;
}

qboolean Pickup_Ammo(Entity *ent, Entity *other)
{
    int         oldcount;
    int         count;
    qboolean    weapon;

    weapon = (ent->item->flags & IT_WEAPON);
    if ((weapon) && ((int)dmflags->value & DeathMatchFlags::InfiniteAmmo))
        count = 1000;
    else if (ent->count)
        count = ent->count;
    else
        count = ent->item->quantity;

    oldcount = other->client->persistent.inventory[ITEM_INDEX(ent->item)];

    if (!Add_Ammo(other, ent->item, count))
        return false;

    if (weapon && !oldcount) {
        if (other->client->persistent.activeWeapon != ent->item && (!deathmatch->value || other->client->persistent.activeWeapon == FindItem("blaster")))
            other->client->newWeapon = ent->item;
    }

    if (!(ent->spawnFlags & (ItemSpawnFlags::DroppedItem | ItemSpawnFlags::DroppedPlayerItem)) && (deathmatch->value))
        SetRespawn(ent, 30);
    return true;
}

void Drop_Ammo(Entity *ent, gitem_t *item)
{
    Entity *dropped;
    int     index;

    index = ITEM_INDEX(item);
    dropped = Drop_Item(ent, item);
    if (ent->client->persistent.inventory[index] >= item->quantity)
        dropped->count = item->quantity;
    else
        dropped->count = ent->client->persistent.inventory[index];

    if (ent->client->persistent.activeWeapon &&
        ent->client->persistent.activeWeapon->tag == AmmoType::Grenade &&
        item->tag == AmmoType::Grenade &&
        ent->client->persistent.inventory[index] - dropped->count <= 0) {
        gi.CPrintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
        G_FreeEntity(dropped);
        return;
    }

    ent->client->persistent.inventory[index] -= dropped->count;
    HUD_ValidateSelectedItem(ent);
}


//======================================================================

void MegaHealth_think(Entity *self)
{
    if (self->owner->health > self->owner->maxHealth) {
        self->nextThink = level.time + 1;
        self->owner->health -= 1;
        return;
    }

    if (!(self->spawnFlags & ItemSpawnFlags::DroppedItem) && (deathmatch->value))
        SetRespawn(self, 20);
    else
        G_FreeEntity(self);
}

qboolean Pickup_Health(Entity *ent, Entity *other)
{
    if (!(ent->style & HEALTH_IGNORE_MAX))
        if (other->health >= other->maxHealth)
            return false;

    other->health += ent->count;

    if (!(ent->style & HEALTH_IGNORE_MAX)) {
        if (other->health > other->maxHealth)
            other->health = other->maxHealth;
    }

    if (ent->style & HEALTH_TIMED) {
        ent->Think = MegaHealth_think;
        ent->nextThink = level.time + 5;
        ent->owner = other;
        ent->flags |= EntityFlags::Respawn;
        ent->serverFlags |= EntityServerFlags::NoClient;
        ent->solid = Solid::Not;
    } else {
        if (!(ent->spawnFlags & ItemSpawnFlags::DroppedItem) && (deathmatch->value))
            SetRespawn(ent, 30);
    }

    return true;
}

//======================================================================

int ArmorIndex(Entity *ent)
{
    if (!ent->client)
        return 0;

    if (ent->client->persistent.inventory[body_armor_index] > 0)
        return body_armor_index;

    return 0;
}

qboolean Pickup_Armor(Entity *ent, Entity *other)
{
    int             old_armor_index;
    gitem_armor_t   *oldinfo;
    gitem_armor_t   *newinfo;
    int             newcount;
    float           salvage;
    int             salvagecount;

    // get info on new armor
    newinfo = (gitem_armor_t *)ent->item->info;

    old_armor_index = ArmorIndex(other);


    // if player has no armor, just use it
    if (!old_armor_index) {
        other->client->persistent.inventory[ITEM_INDEX(ent->item)] = newinfo->baseCount;
    }

    // use the better armor
    else {
        oldinfo = &bodyarmor_info;

        if (newinfo->normalProtection > oldinfo->normalProtection) {
            // calc new armor values
            salvage = oldinfo->normalProtection / newinfo->normalProtection;
            salvagecount = salvage * other->client->persistent.inventory[old_armor_index];
            newcount = newinfo->baseCount + salvagecount;
            if (newcount > newinfo->maxCount)
                newcount = newinfo->maxCount;

            // zero count of old armor so it goes away
            other->client->persistent.inventory[old_armor_index] = 0;

            // change armor to new item with computed value
            other->client->persistent.inventory[ITEM_INDEX(ent->item)] = newcount;
        } else {
            // calc new armor values
            salvage = newinfo->normalProtection / oldinfo->normalProtection;
            salvagecount = salvage * newinfo->baseCount;
            newcount = other->client->persistent.inventory[old_armor_index] + salvagecount;
            if (newcount > oldinfo->maxCount)
                newcount = oldinfo->maxCount;

            // if we're already maxed out then we don't need the new armor
            if (other->client->persistent.inventory[old_armor_index] >= newcount)
                return false;

            // update current armor value
            other->client->persistent.inventory[old_armor_index] = newcount;
        }
    }

    if (!(ent->spawnFlags & ItemSpawnFlags::DroppedItem) && (deathmatch->value))
        SetRespawn(ent, 20);

    return true;
}

//======================================================================

/*
===============
Touch_Item
===============
*/
void Touch_Item(Entity *ent, Entity *other, cplane_t *plane, csurface_t *surf)
{
    qboolean    taken;

    if (!other->client)
        return;
    if (other->health < 1)
        return;     // dead people can't pickup
    if (!ent->item->Pickup)
        return;     // not a grabbable item?

    taken = ent->item->Pickup(ent, other);

    if (taken) {
        // flash the screen
        other->client->bonusAlpha = 0.25;

        // show icon and name on status bar
        other->client->playerState.stats[STAT_PICKUP_ICON] = gi.ImageIndex(ent->item->icon);
        other->client->playerState.stats[STAT_PICKUP_STRING] = ConfigStrings::Items+ ITEM_INDEX(ent->item);
        other->client->pickupMessageTime = level.time + 3.0;

        // change selected item
        if (ent->item->Use)
            other->client->persistent.selectedItem = other->client->playerState.stats[STAT_SELECTED_ITEM] = ITEM_INDEX(ent->item);

        if (ent->item->Pickup == Pickup_Health) {
            if (ent->count == 2)
                gi.Sound(other, CHAN_ITEM, gi.SoundIndex("items/s_health.wav"), 1, ATTN_NORM, 0);
            else if (ent->count == 10)
                gi.Sound(other, CHAN_ITEM, gi.SoundIndex("items/n_health.wav"), 1, ATTN_NORM, 0);
            else if (ent->count == 25)
                gi.Sound(other, CHAN_ITEM, gi.SoundIndex("items/l_health.wav"), 1, ATTN_NORM, 0);
            else // (ent->count == 100)
                gi.Sound(other, CHAN_ITEM, gi.SoundIndex("items/m_health.wav"), 1, ATTN_NORM, 0);
        } else if (ent->item->pickupSound) {
            gi.Sound(other, CHAN_ITEM, gi.SoundIndex(ent->item->pickupSound), 1, ATTN_NORM, 0);
        }
    }

    if (!(ent->spawnFlags & ItemSpawnFlags::TargetsUsed)) {
        UTIL_UseTargets(ent, other);
        ent->spawnFlags |= ItemSpawnFlags::TargetsUsed;
    }

    if (!taken)
        return;

    if (!((coop->value) && (ent->item->flags & IT_STAY_COOP)) || (ent->spawnFlags & (ItemSpawnFlags::DroppedItem | ItemSpawnFlags::DroppedPlayerItem))) {
        if (ent->flags & EntityFlags::Respawn)
            ent->flags &= ~EntityFlags::Respawn;
        else
            G_FreeEntity(ent);
    }
}

//======================================================================

void drop_temp_touch(Entity *ent, Entity *other, cplane_t *plane, csurface_t *surf)
{
    if (other == ent->owner)
        return;

    Touch_Item(ent, other, plane, surf);
}

void drop_make_touchable(Entity *ent)
{
    ent->Touch = Touch_Item;
    if (deathmatch->value) {
        ent->nextThink = level.time + 29;
        ent->Think = G_FreeEntity;
    }
}

Entity *Drop_Item(Entity *ent, gitem_t *item)
{
    Entity *dropped;
    vec3_t  forward, right;
    vec3_t  offset;

    dropped = G_Spawn();

    dropped->className = item->className;
    dropped->item = item;
    dropped->spawnFlags = ItemSpawnFlags::DroppedItem;
    dropped->state.effects = item->worldModelFlags;
    dropped->state.renderfx = RenderEffects::Glow;
    VectorSet(dropped->mins, -15, -15, -15);
    VectorSet(dropped->maxs, 15, 15, 15);
    gi.SetModel(dropped, dropped->item->worldModel);
    dropped->solid = Solid::Trigger;
    dropped->moveType = MoveType::Toss;
    dropped->Touch = drop_temp_touch;
    dropped->owner = ent;

    if (ent->client) {
        trace_t trace;

        AngleVectors(ent->client->aimAngles, &forward, &right, NULL);
        VectorSet(offset, 24, 0, -16);
        dropped->state.origin = G_ProjectSource(ent->state.origin, offset, forward, right);
        trace = gi.Trace(ent->state.origin, dropped->mins, dropped->maxs,
                         dropped->state.origin, ent, CONTENTS_SOLID);
        VectorCopy(trace.endPosition, dropped->state.origin);
    } else {
        AngleVectors(ent->state.angles, &forward, &right, NULL);
        VectorCopy(ent->state.origin, dropped->state.origin);
    }

    VectorScale(forward, 100, dropped->velocity);
    dropped->velocity[2] = 300;

    dropped->Think = drop_make_touchable;
    dropped->nextThink = level.time + 1;

    gi.LinkEntity(dropped);

    return dropped;
}

void Use_Item(Entity *ent, Entity *other, Entity *activator)
{
    ent->serverFlags &= ~EntityServerFlags::NoClient;
    ent->Use = NULL;

    if (ent->spawnFlags & ItemSpawnFlags::NoTouch) {
        ent->solid = Solid::BoundingBox;
        ent->Touch = NULL;
    } else {
        ent->solid = Solid::Trigger;
        ent->Touch = Touch_Item;
    }

    gi.LinkEntity(ent);
}

//======================================================================

/*
================
droptofloor
================
*/
void droptofloor(Entity *ent)
{
    trace_t     tr;
    vec3_t      dest;

    ent->mins = { -15.f, -15.f, -15.f };
    ent->maxs = { 15.f, 15.f, 15.f };

    if (ent->model)
        gi.SetModel(ent, ent->model);
    else
        gi.SetModel(ent, ent->item->worldModel);
    ent->solid = Solid::Trigger;
    ent->moveType = MoveType::Toss;
    ent->Touch = Touch_Item;

    // Calculate trace destination
    dest = ent->state.origin + vec3_t(0.f, 0.f, 128.f);

    tr = gi.Trace(ent->state.origin, ent->mins, ent->maxs, dest, ent, CONTENTS_MASK_SOLID);
    if (tr.startSolid) {
        gi.DPrintf("droptofloor: %s startsolid at %s\n", ent->className, Vec3ToString(ent->state.origin));
        G_FreeEntity(ent);
        return;
    }

    VectorCopy(tr.endPosition, ent->state.origin);

    if (ent->team) {
        ent->flags &= ~EntityFlags::TeamSlave;
        ent->chain = ent->teamChainPtr;
        ent->teamChainPtr = NULL;

        ent->serverFlags |= EntityServerFlags::NoClient;
        ent->solid = Solid::Not;
        if (ent == ent->teamMasterPtr) {
            ent->nextThink = level.time + FRAMETIME;
            ent->Think = DoRespawn;
        }
    }

    if (ent->spawnFlags & ItemSpawnFlags::NoTouch) {
        ent->solid = Solid::BoundingBox;
        ent->Touch = NULL;
        ent->state.effects &= ~EntityEffectType::Rotate;
        ent->state.renderfx &= ~RenderEffects::Glow;
    }

    if (ent->spawnFlags & ItemSpawnFlags::TriggerSpawn) {
        ent->serverFlags |= EntityServerFlags::NoClient;
        ent->solid = Solid::Not;
        ent->Use = Use_Item;
    }

    gi.LinkEntity(ent);
}


/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
void PrecacheItem(gitem_t *it)
{
    const char    *s, *start; // C++20: STRING: Added const
    char    data[MAX_QPATH];
    int     len;
    gitem_t *ammo;

    if (!it)
        return;

    if (it->pickupSound)
        gi.SoundIndex(it->pickupSound);
    if (it->worldModel)
        gi.ModelIndex(it->worldModel);
    if (it->viewModel)
        gi.ModelIndex(it->viewModel);
    if (it->icon)
        gi.ImageIndex(it->icon);

    // parse everything for its ammo
    if (it->ammo && it->ammo[0]) {
        ammo = FindItem(it->ammo);
        if (ammo != it)
            PrecacheItem(ammo);
    }

    // parse the space seperated precache string for other items
    s = it->precaches;
    if (!s || !s[0])
        return;

    while (*s) {
        start = s;
        while (*s && *s != ' ')
            s++;

        len = s - start;
        if (len >= MAX_QPATH || len < 5)
            gi.Error("PrecacheItem: %s has bad precache string", it->className);
        memcpy(data, start, len);
        data[len] = 0;
        if (*s)
            s++;

        // determine type based on extension
        if (!strcmp(data + len - 3, "md2"))
            gi.ModelIndex(data);
        else if (!strcmp(data + len - 3, "sp2"))
            gi.ModelIndex(data);
        else if (!strcmp(data + len - 3, "wav"))
            gi.SoundIndex(data);
        if (!strcmp(data + len - 3, "pcx"))
            gi.ImageIndex(data);
    }
}

/*
============
SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void SpawnItem(Entity *ent, gitem_t *item)
{
    PrecacheItem(item);

    if (ent->spawnFlags) {
        if (strcmp(ent->className, "key_power_cube") != 0) {
            ent->spawnFlags = 0;
            gi.DPrintf("%s at %s has invalid spawnFlags set\n", ent->className, Vec3ToString(ent->state.origin));
        }
    }

    // some items will be prevented in deathmatch
    if (deathmatch->value) {
        if ((int)dmflags->value & DeathMatchFlags::NoArmor) {
            if (item->Pickup == Pickup_Armor) {
                G_FreeEntity(ent);
                return;
            }
        }
        if ((int)dmflags->value & DeathMatchFlags::NoItems) {
            if (item->Pickup == Pickup_Powerup) {
                G_FreeEntity(ent);
                return;
            }
        }
        if ((int)dmflags->value & DeathMatchFlags::NoHealth) {
            if (item->Pickup == Pickup_Health) {
                G_FreeEntity(ent);
                return;
            }
        }
        if ((int)dmflags->value & DeathMatchFlags::InfiniteAmmo) {
            if ((item->flags == IT_AMMO)) {
                G_FreeEntity(ent);
                return;
            }
        }
    }

    if (coop->value && (strcmp(ent->className, "key_power_cube") == 0)) {
        ent->spawnFlags |= (1 << (8 + level.powerCubes));
        level.powerCubes++;
    }

    // don't let them drop items that stay in a coop game
    if ((coop->value) && (item->flags & IT_STAY_COOP)) {
        item->Drop = NULL;
    }

    ent->item = item;
    ent->nextThink = level.time + 2 * FRAMETIME;    // items start after other solids
    ent->Think = droptofloor;
    ent->state.effects = item->worldModelFlags;
    ent->state.renderfx = RenderEffects::Glow;
    if (ent->model)
        gi.ModelIndex(ent->model);
}

//======================================================================

gitem_t itemlist[] = {
    {
        NULL
    },  // leave index 0 alone

    //
    // ARMOR
    //

    /*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        "item_armor_body",
        Pickup_Armor,
        NULL,
        NULL,
        NULL,
        "misc/ar1_pkup.wav",
        "models/items/armor/body/tris.md2", EntityEffectType::Rotate,
        NULL,
        /* icon */      "i_bodyarmor",
        /* pickup */    "Body Armor",
        /* width */     3,
        0,
        NULL,
        IT_ARMOR,
        0,
        &bodyarmor_info,
        ARMOR_BODY,
        /* precache */ ""
    },

    //
    // WEAPONS
    //

    /* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
    always owned, never in the world
    */
    {
        "weapon_blaster",
        NULL,
        Use_Weapon,
        NULL,
        Weapon_Blaster,
        "misc/w_pkup.wav",
        NULL, 0,
        "models/weapons/v_blast/tris.md2",
        /* icon */      "w_blaster",
        /* pickup */    "Blaster",
        0,
        0,
        NULL,
        IT_WEAPON | IT_STAY_COOP,
        WEAP_BLASTER,
        NULL,
        0,
        /* precache */ "weapons/blastf1a.wav misc/lasfly.wav"
    },

    /*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        "weapon_machinegun",
        Pickup_Weapon,
        Use_Weapon,
        Drop_Weapon,
        Weapon_Machinegun,
        "misc/w_pkup.wav",
        "models/weapons/g_machn/tris.md2", EntityEffectType::Rotate,
        "models/weapons/v_machn/tris.md2",
        /* icon */      "w_machinegun",
        /* pickup */    "Machinegun",
        0,
        1,
        "Bullets",
        IT_WEAPON | IT_STAY_COOP,
        WEAP_MACHINEGUN,
        NULL,
        0,
        /* precache */ "weapons/machgf1b.wav weapons/machgf2b.wav weapons/machgf3b.wav weapons/machgf4b.wav weapons/machgf5b.wav"
    },

    /*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        "weapon_shotgun",
        Pickup_Weapon,
        Use_Weapon,
        Drop_Weapon,
        Weapon_Shotgun,
        "misc/w_pkup.wav",
        "models/weapons/g_shotg/tris.md2", EntityEffectType::Rotate,
        "models/weapons/v_shotg/tris.md2",
        /* icon */      "w_shotgun",
        /* pickup */    "Shotgun",
        0,
        1,
        "Shells",
        IT_WEAPON | IT_STAY_COOP,
        WEAP_SHOTGUN,
        NULL,
        0,
        /* precache */ "weapons/shotgf1b.wav weapons/shotgr1b.wav"
    },

    /*QUAKED weapon_supershotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        "weapon_supershotgun",
        Pickup_Weapon,
        Use_Weapon,
        Drop_Weapon,
        Weapon_SuperShotgun,
        "misc/w_pkup.wav",
        "models/weapons/g_shotg2/tris.md2", EntityEffectType::Rotate,
        "models/weapons/v_shotg2/tris.md2",
        /* icon */      "w_sshotgun",
        /* pickup */    "Super Shotgun",
        0,
        2,
        "Shells",
        IT_WEAPON | IT_STAY_COOP,
        WEAP_SUPERSHOTGUN,
        NULL,
        0,
        /* precache */ "weapons/sshotf1b.wav"
    },

    //
    // AMMO ITEMS
    //

    /*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        "ammo_shells",
        Pickup_Ammo,
        NULL,
        Drop_Ammo,
        NULL,
        "misc/am_pkup.wav",
        "models/items/ammo/shells/medium/tris.md2", 0,
        NULL,
        /* icon */      "a_shells",
        /* pickup */    "Shells",
        /* width */     3,
        10,
        NULL,
        IT_AMMO,
        0,
        NULL,
        AmmoType::Shells,
        /* precache */ ""
    },

    /*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        "ammo_bullets",
        Pickup_Ammo,
        NULL,
        Drop_Ammo,
        NULL,
        "misc/am_pkup.wav",
        "models/items/ammo/bullets/medium/tris.md2", 0,
        NULL,
        /* icon */      "a_bullets",
        /* pickup */    "Bullets",
        /* width */     3,
        50,
        NULL,
        IT_AMMO,
        0,
        NULL,
        AmmoType::Bullets,
        /* precache */ ""
    },

    {
        NULL,
        Pickup_Health,
        NULL,
        NULL,
        NULL,
        "items/pkup.wav",
        NULL, 0,
        NULL,
        /* icon */      "i_health",
        /* pickup */    "Health",
        /* width */     3,
        0,
        NULL,
        0,
        0,
        NULL,
        0,
        /* precache */ "items/s_health.wav items/n_health.wav items/l_health.wav items/m_health.wav"
    },

    // end of list marker
    {NULL}
};


/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health(Entity *self)
{
    if (deathmatch->value && ((int)dmflags->value & DeathMatchFlags::NoHealth)) {
        G_FreeEntity(self);
        return;
    }

    self->model = "models/items/healing/medium/tris.md2";
    self->count = 10;
    SpawnItem(self, FindItem("Health"));
    gi.SoundIndex("items/n_health.wav");
}

/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_small(Entity *self)
{
    if (deathmatch->value && ((int)dmflags->value & DeathMatchFlags::NoHealth)) {
        G_FreeEntity(self);
        return;
    }

    self->model = "models/items/healing/stimpack/tris.md2";
    self->count = 2;
    SpawnItem(self, FindItem("Health"));
    self->style = HEALTH_IGNORE_MAX;
    gi.SoundIndex("items/s_health.wav");
}

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_large(Entity *self)
{
    if (deathmatch->value && ((int)dmflags->value & DeathMatchFlags::NoHealth)) {
        G_FreeEntity(self);
        return;
    }

    self->model = "models/items/healing/large/tris.md2";
    self->count = 25;
    SpawnItem(self, FindItem("Health"));
    gi.SoundIndex("items/l_health.wav");
}

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_mega(Entity *self)
{
    if (deathmatch->value && ((int)dmflags->value & DeathMatchFlags::NoHealth)) {
        G_FreeEntity(self);
        return;
    }

    self->model = "models/items/mega_h/tris.md2";
    self->count = 100;
    SpawnItem(self, FindItem("Health"));
    gi.SoundIndex("items/m_health.wav");
    self->style = HEALTH_IGNORE_MAX | HEALTH_TIMED;
}


void InitItems(void)
{
    game.numberOfItems = sizeof(itemlist) / sizeof(itemlist[0]) - 1;
}



/*
===============
SetItemNames

Called by worldspawn
===============
*/
void SetItemNames(void)
{
    int     i;
    gitem_t *it;

    for (i = 0 ; i < game.numberOfItems ; i++) {
        it = &itemlist[i];
        gi.configstring(ConfigStrings::Items+ i, it->pickupName);
    }

    body_armor_index   = ITEM_INDEX(FindItem("Body Armor"));
}
