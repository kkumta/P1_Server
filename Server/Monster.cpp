#include "pch.h"
#include "Monster.h"

Monster::Monster()
{
	objectInfo->set_creature_type(Protocol::CreatureType::CREATURE_TYPE_MONSTER);

	monsterInfo = new Protocol::MonsterInfo();
	objectInfo->set_allocated_monster_info(monsterInfo);
}

Monster::~Monster()
{
}