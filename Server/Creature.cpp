#include "pch.h"
#include "Creature.h"

Creature::Creature()
{
	objectInfo->set_object_type(Protocol::ObjectType::OBJECT_TYPE_CREATURE);

	creatureInfo = new Protocol::CreatureInfo();
	objectInfo->set_allocated_creature_info(creatureInfo);
}

Creature::~Creature()
{

}