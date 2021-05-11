//
// This file is rather annoying/complicated, we'll have to seek for some way to replace it.
// 
// For now though, it works so... keep it simple stupid, don't change it if it ain't broken :)
//
#include "g_local.h"
#include "effects.h"
#include "utils.h"

// 
// ---- generated by grep & sed, do not edit = WRONG!!! Haha
// This wasn't generated anymore at all in our code, ah well :P
//
#include "functionpointers.h"

extern mmove_t soldier_move_attack1;
extern mmove_t soldier_move_attack2;
extern mmove_t soldier_move_attack3;
extern mmove_t soldier_move_attack4;
extern mmove_t soldier_move_attack6;
extern mmove_t soldier_move_death1;
extern mmove_t soldier_move_death2;
extern mmove_t soldier_move_death3;
extern mmove_t soldier_move_death4;
extern mmove_t soldier_move_death5;
extern mmove_t soldier_move_death6;
extern mmove_t soldier_move_duck;
extern mmove_t soldier_move_pain1;
extern mmove_t soldier_move_pain2;
extern mmove_t soldier_move_pain3;
extern mmove_t soldier_move_pain4;
extern mmove_t soldier_move_run;
extern mmove_t soldier_move_stand1;
extern mmove_t soldier_move_stand3;
extern mmove_t soldier_move_start_run;
extern mmove_t soldier_move_walk1;
extern mmove_t soldier_move_walk2;

extern void door_blocked(Entity* self, Entity* other);
extern void plat_blocked(Entity* self, Entity* other);
extern void rotating_blocked(Entity* self, Entity* other);
extern void train_blocked(Entity* self, Entity* other);
extern void body_die(Entity* self, Entity* inflictor, Entity* attacker, int damage, const vec3_t& point);
extern void button_killed(Entity* self, Entity* inflictor, Entity* attacker, int damage, const vec3_t& point);
extern void debris_die(Entity* self, Entity* inflictor, Entity* attacker, int damage, const vec3_t& point);
extern void door_killed(Entity* self, Entity* inflictor, Entity* attacker, int damage, const vec3_t& point);
extern void func_explosive_explode(Entity* self, Entity* inflictor, Entity* attacker, int damage, const vec3_t& point);
extern void gib_die(Entity* self, Entity* inflictor, Entity* attacker, int damage, const vec3_t& point);
extern void soldier_die(Entity* self, Entity* inflictor, Entity* attacker, int damage, const vec3_t& point);

extern void soldier_attack(Entity *self);
extern void soldier_dodge(Entity* self, Entity* attacker, float eta);
extern void soldier_run(Entity *self);
extern void soldier_sight(Entity* self, Entity* other);
extern void soldier_stand(Entity *self);
extern void soldier_walk(Entity *self);
extern void soldier_pain(Entity* self, Entity* other, float kick, int damage);
;
extern void Brush_AngleMove_Begin(Entity* self);
extern void Brush_AngleMove_Done(Entity* self);
extern void Brush_AngleMove_Final(Entity* self);

extern void button_return(Entity* self);

extern void door_go_down(Entity* self);

extern void DoRespawn(Entity* self);
extern void drop_make_touchable(Entity* self);
extern void droptofloor(Entity* self);
extern void flymonster_start_go(Entity* self);

extern void func_object_release(Entity* self);
extern void func_timer_think(Entity* self);
extern void func_train_find(Entity* self);

extern void gib_think(Entity* self);

extern void MegaHealth_think(Entity* self);

extern void monster_triggered_spawn(Entity* self);
extern void Brush_Move_Begin(Entity* self);
extern void Brush_Move_Done(Entity* self);
extern void Brush_Move_Final(Entity* self);
extern void multi_wait(Entity* self);
extern void plat_go_down(Entity* self);
extern void SP_CreateCoopSpots(Entity* self);
extern void SP_FixCoopSpots(Entity* self);
extern void swimmonster_start_go(Entity* self);
extern void target_crosslevel_target_think(Entity* self);
extern void target_earthquake_think(Entity* self);
extern void target_explosion_explode(Entity* self);
extern void target_lightramp_think(Entity* self);
extern void Think_AccelMove(Entity* self);

extern void Think_CalcMoveSpeed(Entity* self);
extern void Think_Delay(Entity* self);
extern void Think_SpawnDoorTrigger(Entity* self);
extern void train_next(Entity* self);
extern void trigger_elevator_init(Entity* self);
extern void walkmonster_start_go(Entity* self);
extern void blaster_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void button_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void door_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void drop_temp_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void func_object_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void gib_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void hurt_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);

extern void rotating_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);

extern void teleporter_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void Touch_DoorTrigger(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void Touch_Multi(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void Touch_Plat_Center(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void trigger_gravity_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void trigger_monsterjump_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void trigger_push_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);

extern void button_use(Entity* self, Entity* other, Entity* activator);
extern void door_use(Entity* self, Entity* other, Entity* activator);
extern void func_conveyor_use(Entity* self, Entity* other, Entity* activator);
extern void func_explosive_spawn(Entity* self, Entity* other, Entity* activator);
extern void func_explosive_use(Entity* self, Entity* other, Entity* activator);
extern void func_object_use(Entity* self, Entity* other, Entity* activator);
extern void func_timer_use(Entity* self, Entity* other, Entity* activator);
extern void func_wall_use(Entity* self, Entity* other, Entity* activator);
extern void hurt_use(Entity* self, Entity* other, Entity* activator);
extern void light_use(Entity* self, Entity* other, Entity* activator);
extern void monster_triggered_spawn_use(Entity* self, Entity* other, Entity* activator);
extern void monster_use(Entity* self, Entity* other, Entity* activator);
extern void rotating_use(Entity* self, Entity* other, Entity* activator);
extern void target_earthquake_use(Entity* self, Entity* other, Entity* activator);
extern void target_lightramp_use(Entity* self, Entity* other, Entity* activator);
extern void train_use(Entity* self, Entity* other, Entity* activator);
extern void trigger_counter_use(Entity* self, Entity* other, Entity* activator);
extern void trigger_crosslevel_trigger_use(Entity* self, Entity* other, Entity* activator);
extern void trigger_elevator_use(Entity* self, Entity* other, Entity* activator);
extern void trigger_enable(Entity* self, Entity* other, Entity* activator);
extern void trigger_key_use(Entity* self, Entity* other, Entity* activator);
extern void trigger_relay_use(Entity* self, Entity* other, Entity* activator);
extern void Use_Areaportal(Entity* self, Entity* other, Entity* activator);

extern void Use_Item(Entity* ent, Entity* other, Entity* activator);
extern void use_killbox(Entity* ent, Entity* other, Entity* activator);
extern void Use_Multi(Entity* ent, Entity* other, Entity* activator);
extern void Use_Plat(Entity* ent, Entity* other, Entity* activator);
extern void use_target_blaster(Entity* ent, Entity* other, Entity* activator);
extern void use_target_changelevel(Entity* ent, Entity* other, Entity* activator);
extern void use_target_explosion(Entity* ent, Entity* other, Entity* activator);
extern void use_target_spawner(Entity* ent, Entity* other, Entity* activator);
extern void Use_Target_Speaker(Entity* ent, Entity* other, Entity* activator);
extern void use_target_splash(Entity* ent, Entity* other, Entity* activator);
extern void Use_Target_Tent(Entity* ent, Entity* other, Entity* activator);
extern void plat_hit_bottom(Entity* self);
extern void plat_hit_top(Entity* self);
extern void button_done(Entity* self);
extern void button_wait(Entity* self);
extern void door_hit_bottom(Entity* self);
extern void door_hit_top(Entity* self);
extern void train_wait(Entity* self);

extern void barrel_explode(Entity* self);
extern void barrel_touch(Entity* self, Entity* other, cplane_t* plane, csurface_t* surf);
extern void barrel_delay(Entity* self, Entity* inflictor, Entity* attacker, int damage, const vec3_t& point);

const save_ptr_t save_ptrs[] = {
{ P_blocked, door_blocked },
{ P_blocked, plat_blocked },
{ P_blocked, rotating_blocked },
{ P_blocked, train_blocked },

{ P_die, body_die },
{ P_die, button_killed },
{ P_die, debris_die },
{ P_die, door_killed },
	{ P_die, barrel_delay },

{ P_die, func_explosive_explode },

{ P_die, gib_die },
{ P_die, Player_Die },
{ P_die, soldier_die },

{ P_monsterinfo_attack, soldier_attack },

{ P_monsterinfo_checkattack, M_CheckAttack },

{ P_monsterinfo_currentmove, &soldier_move_attack1 },
{ P_monsterinfo_currentmove, &soldier_move_attack2 },
{ P_monsterinfo_currentmove, &soldier_move_attack3 },
{ P_monsterinfo_currentmove, &soldier_move_attack4 },
{ P_monsterinfo_currentmove, &soldier_move_attack6 },
{ P_monsterinfo_currentmove, &soldier_move_death1 },
{ P_monsterinfo_currentmove, &soldier_move_death2 },
{ P_monsterinfo_currentmove, &soldier_move_death3 },
{ P_monsterinfo_currentmove, &soldier_move_death4 },
{ P_monsterinfo_currentmove, &soldier_move_death5 },
{ P_monsterinfo_currentmove, &soldier_move_death6 },
{ P_monsterinfo_currentmove, &soldier_move_duck },
{ P_monsterinfo_currentmove, &soldier_move_pain1 },
{ P_monsterinfo_currentmove, &soldier_move_pain2 },
{ P_monsterinfo_currentmove, &soldier_move_pain3 },
{ P_monsterinfo_currentmove, &soldier_move_pain4 },
{ P_monsterinfo_currentmove, &soldier_move_run },
{ P_monsterinfo_currentmove, &soldier_move_stand1 },
{ P_monsterinfo_currentmove, &soldier_move_stand3 },
{ P_monsterinfo_currentmove, &soldier_move_start_run },
{ P_monsterinfo_currentmove, &soldier_move_walk1 },
{ P_monsterinfo_currentmove, &soldier_move_walk2 },


{ P_monsterinfo_dodge, soldier_dodge },

{ P_monsterinfo_run, soldier_run },

{ P_monsterinfo_sight, soldier_sight },

{ P_monsterinfo_stand, soldier_stand },

{ P_monsterinfo_walk, soldier_walk },

{ P_pain, Player_Pain },
{ P_pain, soldier_pain },

{ P_think, Brush_AngleMove_Begin },
{ P_think, Brush_AngleMove_Done },
{ P_think, Brush_AngleMove_Final },

{ P_think, button_return },

{ P_think, door_go_down },

{ P_think, DoRespawn },
{ P_think, drop_make_touchable },
{ P_think, droptofloor },
{ P_think, flymonster_start_go },
{ P_think, func_object_release },
{ P_think, func_timer_think },
{ P_think, func_train_find },
{ P_think, G_FreeEntity },
{ P_think, gib_think },

{ P_think, M_droptofloor },
{ P_think, MegaHealth_think },
{ P_think, monster_think },
{ P_think, monster_triggered_spawn },
{ P_think, Brush_Move_Begin },
{ P_think, Brush_Move_Done },
{ P_think, Brush_Move_Final },
{ P_think, multi_wait },
{ P_think, plat_go_down },
{ P_think, SP_CreateCoopSpots },
{ P_think, SP_FixCoopSpots },
{ P_think, swimmonster_start_go },
{ P_think, target_crosslevel_target_think },
{ P_think, target_earthquake_think },
{ P_think, target_explosion_explode },
{ P_think, target_lightramp_think },
{ P_think, Think_AccelMove },

	{ P_think, barrel_explode},

{ P_think, Think_CalcMoveSpeed },
{ P_think, Think_Delay },
{ P_think, Think_SpawnDoorTrigger },
{ P_think, train_next },
{ P_think, trigger_elevator_init },
{ P_think, walkmonster_start_go },
{ P_touch, blaster_touch },
{ P_touch, button_touch },
{ P_touch, door_touch },
{ P_touch, drop_temp_touch },
{ P_touch, func_object_touch },
{ P_touch, gib_touch },
{ P_touch, hurt_touch },
	{ P_touch, barrel_touch },
{ P_touch, rotating_touch },


{ P_touch, teleporter_touch },

{ P_touch, Touch_DoorTrigger },
{ P_touch, Touch_Item },
{ P_touch, Touch_Multi },
{ P_touch, Touch_Plat_Center },

{ P_touch, trigger_gravity_touch },
{ P_touch, trigger_monsterjump_touch },
{ P_touch, trigger_push_touch },

{ P_use, button_use },

{ P_use, door_use },

{ P_use, func_conveyor_use },
{ P_use, func_explosive_spawn },
{ P_use, func_explosive_use },
{ P_use, func_object_use },
{ P_use, func_timer_use },
{ P_use, func_wall_use },

{ P_use, hurt_use },
{ P_use, light_use },

{ P_use, monster_triggered_spawn_use },
{ P_use, monster_use },

{ P_use, rotating_use },

{ P_use, target_earthquake_use },
{ P_use, target_lightramp_use },
{ P_use, train_use },

{ P_use, trigger_counter_use },
{ P_use, trigger_crosslevel_trigger_use },
{ P_use, trigger_elevator_use },
{ P_use, trigger_enable },
{ P_use, trigger_key_use },
{ P_use, trigger_relay_use },

{ P_use, Use_Areaportal },
{ P_use, Use_Item },
{ P_use, use_killbox },
{ P_use, Use_Multi },
{ P_use, Use_Plat },
{ P_use, use_target_blaster },
{ P_use, use_target_changelevel },
{ P_use, use_target_explosion },
{ P_use, use_target_spawner },
{ P_use, Use_Target_Speaker },
{ P_use, use_target_splash },
{ P_use, Use_Target_Tent },

{ P_moveinfo_endfunc, plat_hit_bottom },
{ P_moveinfo_endfunc, plat_hit_top },

{ P_moveinfo_endfunc, button_done },
{ P_moveinfo_endfunc, button_wait },

{ P_moveinfo_endfunc, door_hit_bottom },
{ P_moveinfo_endfunc, door_hit_top },

{ P_moveinfo_endfunc, train_wait },

};
const int num_save_ptrs = sizeof(save_ptrs) / sizeof(save_ptrs[0]);
