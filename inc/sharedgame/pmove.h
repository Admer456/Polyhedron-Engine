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

#ifndef SHAREDGAME_PMOVE_H
#define SHAREDGAME_PMOVE_H

// Shared include, we need it :)
#include "shared/shared.h"

//
//==============================================================
//
// Player Movement(PM) Code.
//
// Common between server and client so prediction matches
//
//==============================================================
//

//
// Acceleration Constants.
//
#define PM_ACCEL_AIR			2.125f
#define PM_ACCEL_AIR_MOD_DUCKED	0.125f
#define PM_ACCEL_GROUND			10.f
#define PM_ACCEL_GROUND_SLICK	4.375f
#define PM_ACCEL_LADDER			16.f
#define PM_ACCEL_SPECTATOR		2.5f
#define PM_ACCEL_WATER			2.8f

//
// Bounce constant when clipping against solids.
//
#define PM_CLIP_BOUNCE			1.01f

//
// Friction constants.
//
#define PM_FRICT_AIR			0.1f
#define PM_FRICT_GROUND			6.f
#define PM_FRICT_GROUND_SLICK	2.f
#define PM_FRICT_LADDER			5.f
#define PM_FRICT_SPECTATOR		2.5f
#define PM_FRICT_WATER			2.f

//
// Water gravity constant.
//
#define PM_GRAVITY_WATER		0.33f

//
// Distances traced when seeking ground.
//
#define PM_GROUND_DIST			.25f
#define PM_GROUND_DIST_TRICK	16.f

//
// Speed constants; intended velocities are clipped to these.
//
#define PM_SPEED_AIR			350.f
#define PM_SPEED_CURRENT		100.f
#define PM_SPEED_DUCK_STAND		200.f
#define PM_SPEED_DUCKED			140.f
#define PM_SPEED_FALL			-700.f
#define PM_SPEED_FALL_FAR		-900.f
#define PM_SPEED_JUMP			270.f
#define PM_SPEED_LADDER			125.f
#define PM_SPEED_LAND			-280.f
#define PM_SPEED_RUN			300.f
#define PM_SPEED_SPECTATOR		500.f
#define PM_SPEED_STOP			100.f
#define PM_SPEED_UP				0.1f
#define PM_SPEED_TRICK_JUMP		0.f
#define PM_SPEED_WATER			118.f
#define PM_SPEED_WATER_JUMP		420.f
#define PM_SPEED_WATER_SINK		-16.f

//
// The walk modifier slows all user-controlled speeds.
//
#define PM_SPEED_MOD_WALK		0.66f

//
// Water reduces jumping ability.
//
#define PM_SPEED_JUMP_MOD_WATER	0.66f

//
// The vertical distance afforded in step climbing.
//
#define PM_STEP_HEIGHT			16.f

//
// The smallest step that will be interpolated by the client.
//
#define PM_STEP_HEIGHT_MIN		4.f

//
// The minimum Z plane normal component required for standing.
//
#define PM_STEP_NORMAL			0.7f

//
// Velocity is cleared when less than this.
//
#define PM_STOP_EPSILON			0.1f

//
// Invalid player positions are nudged to find a valid position.
//
#define PM_NUDGE_DIST			1.f

//
// Valid player positions are snapped a small distance away from planes.
//
#define PM_SNAP_DISTANCE		PM_GROUND_DIST

//
// Player bounding box scaling. mins = Vec3_Scale(PM_MINS, PM_SCALE)..
//
#define PM_SCALE 1.f

extern const vec3_t PM_MINS;
extern const vec3_t PM_MAXS;

//
// Game-specific button hits.
//
//#define BUTTON_ATTACK		(1 << 0)
//#define BUTTON_WALK			(1 << 1)
//#define BUTTON_HOOK			(1 << 2)
//#define BUTTON_SCORE		(1 << 3)

//
// Game-specific flags for pm_state_t.flags.
//
// Player Move Flags.
#define PMF_GAME			(1 << 0)
#define PMF_DUCKED          (PMF_GAME << 0) // Player is ducked.
#define PMF_JUMP_HELD       (PMF_GAME << 1) // Player jump key is held.
#define PMF_ON_GROUND       (PMF_GAME << 2) // Player is on-ground.
#define PMF_ON_STAIRS		(PMF_GAME << 3) // Player is traversing stairs.
#define PMF_TIME_WATERJUMP  (PMF_GAME << 4) // Value of: time is waterjump
#define PMF_TIME_LAND       (PMF_GAME << 5) // Value of: time is time before rejump
#define PMF_TIME_TELEPORT   (PMF_GAME << 6) // Value of: time is non-moving time
#define PMF_NO_PREDICTION   (PMF_GAME << 7) // Temporarily disables prediction (used for grappling hook)
#define PMF_TELEPORT_BIT    (PMF_GAME << 8) // Used by q2pro
// NOTE: Any more flags requires changing the pmove states and networking code.

//#define PMF_DUCKED				(PMF_GAME << 0) // player is ducked
//#define PMF_JUMPED				(PMF_GAME << 1) // player jumped
//#define PMF_JUMP_HELD			(PMF_GAME << 2) // player's jump key is down
//#define PMF_ON_GROUND			(PMF_GAME << 3) // player is on ground
//#define PMF_ON_STAIRS			(PMF_GAME << 4) // player traversed step
//#define PMF_ON_LADDER			(PMF_GAME << 5) // player is on ladder
//#define PMF_UNDER_WATER			(PMF_GAME << 6) // player is under water
//#define PMF_TIME_PUSHED			(PMF_GAME << 7) // time before can seek ground
//#define PMF_TIME_TRICK_JUMP		(PMF_GAME << 8) // time eligible for trick jump
//#define PMF_TIME_WATER_JUMP		(PMF_GAME << 9) // time before control
//#define PMF_TIME_LAND			(PMF_GAME << 10) // time before jump eligible
//#define PMF_TIME_TELEPORT		(PMF_GAME << 11) // time frozen in place
//#define PMF_GIBLET				(PMF_GAME << 12) // player is a giblet
//#define PMF_HOOK_RELEASED		(PMF_GAME << 13) // player's hook key was released
//#define PMF_TIME_TRICK_START	(PMF_GAME << 14) // time until we can initiate a trick jump

//
// The mask of pm_state_t.flags affecting pm_state_t.time.
//
#define PMF_TIME_MASK ( \
                        PMF_TIME_PUSHED | \
                        PMF_TIME_TRICK_JUMP | \
                        PMF_TIME_WATER_JUMP | \
                        PMF_TIME_LAND | \
                        PMF_TIME_TELEPORT | \
						PMF_TIME_TRICK_START \
                      )

//
// The maximum number of entities any single player movement can impact.
//
#define PM_MAX_TOUCH_ENTS 32


//-------------------
// The player movement structure provides context management between the
// game modules and the player movement code.
// 
// (in), (out), (in/out) mark which way a variable goes. Copied in to the
// state befor processing, or copied back out of the state after processing.
//-------------------
typedef struct {
    // Movement command (in)
    usercmd_t cmd;  

    // Movement state (in/out)
    pm_state_t state;

    // TODO: Do we need this still?
    // If .s has changed outside of pmove, testInitial is true (in)
    qboolean        testInitial;

    // Hook pull speed (in)
    float hookPullSpeed;

    // Entities touched (out)
    struct edict_s* touchedEntities[PM_MAX_TOUCH_ENTS];
    int32_t numTouchedEntities;

    // Pointer to the entity that is below the player. (out)
    struct edict_s* groundEntity; 

    // Clamped, and including kick and delta (out)
    vec3_t viewAngles;

    // Bounding box size (out)
    vec3_t mins, maxs;
    
    float       step; // Traversed step height. (out)

    // Water type and level (Ranges 0 - 3, 0 = no water..)
    int         waterType;  
    int         waterLevel; // Water Level (1 - 3)

    // Callback functions for collision with the world and solid entities
    trace_t (*q_gameabi Trace)(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
    int     (*PointContents)(vec3_t point);



    //int             numtouch;               // Number of touched entities.
    //struct edict_s* touchents[MAXTOUCH];   // Pointers to touched entities.

    //struct edict_s* groundentity; // Pointer to the entity that is below the player.

    //vec3_t      viewangles; // Clamped View Angles
    //float       viewheight; // Viewheight.

    //vec3_t      mins, maxs; // Bounding box size

} pm_move_t;




//-------------------
// This is a shared structure between the client and server.
// Each client has their own individual pmove parameters.
// To add your own, add them below the comment.
//-------------------
typedef struct {
    qboolean    qwmode;
    qboolean    airaccelerate;
    qboolean    strafehack;
    qboolean    flyhack;
    qboolean    waterhack;
    float       speedmult;
    float       watermult;
    float       maxspeed;
    float       friction;
    float       waterfriction;
    float       flyfriction;
} pmoveParams_t;


//
// PMove functions.
//
void PMove(pm_move_t* pmove, pmoveParams_t* params);

// TODO: 
void PMoveInit(pmoveParams_t* pmp);
void PMoveEnableQW(pmoveParams_t* pmp);

#endif // PMOVE_H
