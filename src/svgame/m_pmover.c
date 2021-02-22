/*
===========================================================================
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2000-2002 Mr. Hyde and Mad Dog

This file is part of Lazarus Quake 2 Mod source code.

Lazarus Quake 2 Mod source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Lazarus Quake 2 Mod source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Lazarus Quake 2 Mod source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*
==============================================================================

PMover - AI using Player Movement.

==============================================================================
*/

#include "g_local.h"
#include "g_pmai.h"
#include "m_pmover.h"

// AI Player Move settings are stored here.
pmoveParams_t aipmp;

//
// NON IMPORTANT, ARE IN SHARED/SHARED.H but the GAME DLL IS STILL RIGGED WITH THAT KMQ2 ISSUE.
//
#define ANGLE2SHORT(x)  ((int)((x)*65536/360) & 65535)
#define SHORT2ANGLE(x)  ((x)*(360.0/65536))

//
// NON IMPORTANT FUNCTION.
//
void PMover_Pain(edict_t* self, edict_t* other, float kick, int damage)
{

}

//
// NON IMPORTANT FUNCTION.
//
void PMover_Die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	self->s.effects = 0;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;

	self->s.skinnum |= 1;

	if (self->deadflag == DEAD_DEAD)
		return;

	// regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
}

//
// This is a straight copy from PM_Trace.
// Eventually it needs to be moved into its own file obviously.
//
edict_t* pmai_passent;
trace_t	PMAI_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	if (pmai_passent->health > 0)
		return gi.trace(start, mins, maxs, end, pmai_passent, MASK_PLAYERSOLID);
	else
		return gi.trace(start, mins, maxs, end, pmai_passent, MASK_DEADSOLID);
}

//
// Does the thinking for the pmover...
//

// X(PITCH) = 0
// Y(YAW) = 1
// Z(ROLL) = 2

void PMover_Think(edict_t* self) {
	edict_t* target = NULL;

	// Clear the user input.
	usercmd_t aicmd;
	memset(&aicmd, 0, sizeof(aicmd));

	// Each frame we look for an enemy.
	PMAI_FindEnemyTarget(self);

	// If we've found an enemy target. Proceed further.
	if (self->pmai.targets.enemy.entity) {
		vec3_t vecyaw;
		edict_t* target = self->pmai.targets.enemy.entity;

		// Calculate the yaw to move to.
		VectorSubtract(target->s.origin, self->s.origin, vecyaw);
		float yaw = vectoyaw2(vecyaw);

		// Tell it to move.
		aicmd.forwardmove = 240;
		aicmd.msec = 30;
		aicmd.angles[YAW] = ANGLE2SHORT(yaw);
//		aicmd.angles[0] = ANGLE2SHORT(self->ideal_yaw);
//		aicmd.angles[1] = ANGLE2SHORT(self->ideal_yaw);
//		aicmd.angles[2] = ANGLE2SHORT(self->ideal_yaw);

		self->s.angles[1] = self->ideal_yaw;
	}

	//-------------------------------------------------------------------------
	// Actual movement code.
	//-------------------------------------------------------------------------

	// Setup a user input command for this AI frame.
	//aicmd.forwardmove = 80;
	//aicmd.msec = 30;
	//aicmd.angles[0] = ANGLE2SHORT(self->s.angles[0]);
	//aicmd.angles[1] = ANGLE2SHORT(self->s.angles[1]);
	//aicmd.angles[2] = ANGLE2SHORT(self->s.angles[2]);
	
	// Execute the player movement using the given "AI Player Input"
	pmai_passent = self;		// Store self in pm_passent
	self->pmai.pmove.cmd = aicmd;		// Copy over ai movement cmd.
	
	Pmove(&self->pmai.pmove, &aipmp);	// Execute!

	// Unlink the entity, copy origin, relink it.
	gi.unlinkentity(self);
	VectorCopy(self->pmai.pmove.s.origin, self->s.origin);
	gi.linkentity(self);

	// Setup the next think.
	self->nextthink = level.time + 0.1;
	self->think = PMover_Think;
}

/*QUAKED monster_pmover (1 .5 0) (-16 -16 -24) (16 16 32) 
*/
void SP_monster_pmover(edict_t* self)
{
	// Set movetype, solid, model, and bounds.
	self->movetype = MOVETYPE_WALK;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/infantry/tris.md2");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 32);

	// Setup edict entity functions.
	self->pain = PMover_Pain;
	self->die = PMover_Die;

	// Set health and "flies".
	self->health = 100;
	if (!self->monsterinfo.flies)
		self->monsterinfo.flies = 0.10;

	// Setup class description.
	self->common_name = "Pmover";
	self->class_id = ENTITY_MONSTER_PMOVER;

	// Setup the think.
	self->nextthink = level.time + 0.1;
	self->think = PMover_Think;

	//-------------------------------------------------------------------------
	// Setup actual player movement.
	//-------------------------------------------------------------------------
	// Initialize the AI Player Move settings.
	// Cheap trick using a static to preven it from happening again.
	static qboolean aipmp_is_initialized = false;
	if (aipmp_is_initialized == false) {
		PmoveInit(&aipmp);
		aipmp_is_initialized = true;
	}

	// Setup the pmove trace and point contents function pointers.
	self->pmai.pmove.trace = PMAI_Trace;	// adds default parms
	self->pmai.pmove.pointcontents = gi.pointcontents;

	// Setup the pmove bounding box.
	VectorSet(self->pmai.pmove.mins, -16, -16, -24);
	VectorSet(self->pmai.pmove.maxs, 16, 16, 32);

	// Setup the pmove state flags.
	self->pmai.pmove.s.pm_flags &= ~PMF_NO_PREDICTION;	// We don't want it to use prediction, there is no client.
	self->pmai.pmove.s.gravity = sv_gravity->value;		// Default gravity.
	self->pmai.pmove.s.pm_type = PM_NORMAL;				// Defualt Player Movement.
	self->pmai.pmove.s.pm_time = 1;						// 1ms = 8 units
	
	// Copy over the entities origin into the player move for its spawn point.
	VectorCopy(self->s.origin, self->pmai.pmove.s.origin);

	// Setup other entity data.
	self->classname = "pmover";
	self->class_id = ENTITY_MONSTER_PMOVER;
	self->deadflag = DEAD_NO;
	self->groundentity = NULL;
	self->takedamage = DAMAGE_AIM;
	self->mass = 800;
	self->flags &= ~FL_NO_KNOCKBACK;
	self->svflags &= ~SVF_NOCLIENT;		// Let the server know that this is not a client either.
	self->clipmask = MASK_PLAYERSOLID;	// We want clipping to behave as if it is a player.

	// Link entity to world.
	gi.linkentity(self);
}
