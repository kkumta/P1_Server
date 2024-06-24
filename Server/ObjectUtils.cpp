#include "pch.h"
#include "ObjectUtils.h"
#include "Player.h"
#include "Monster.h"
#include "GameSession.h"

atomic<int64> ObjectUtils::s_idGenerator = 1;

PlayerPtr ObjectUtils::CreatePlayer(GameSessionPtr session, string nickname)
{
	const int64 newId = s_idGenerator.fetch_add(1);

	PlayerPtr player = make_shared<Player>();

	player->objectInfo->set_object_id(newId);
	player->objectInfo->set_nickname(nickname);

	player->posInfo->set_object_id(newId);

    player->creatureInfo->set_object_id(newId);
    player->creatureInfo->set_max_hp(PLAYER_MAX_HP);
    player->creatureInfo->set_cur_hp(PLAYER_MAX_HP);
    player->creatureInfo->set_damage(PLAYER_DAMAGE);

	player->session = session;
	session->player.store(player);

	return player;
}

MonsterPtr ObjectUtils::CreateMonster(MonsterPtr monster)
{
    const int64_t newId = s_idGenerator.fetch_add(1);

    MonsterPtr newMonster = make_shared<Monster>();

    newMonster->objectInfo->set_object_id(newId);
    newMonster->objectInfo->set_nickname(monster->objectInfo->nickname());

    newMonster->posInfo->set_object_id(newId);
    newMonster->posInfo->set_x(monster->posInfo->x());
    newMonster->posInfo->set_y(monster->posInfo->y());
    newMonster->posInfo->set_z(monster->posInfo->z());
    newMonster->posInfo->set_yaw(monster->posInfo->yaw());

    newMonster->creatureInfo->set_object_id(newId);
    newMonster->creatureInfo->set_max_hp(MONSTER_MAX_HP);
    newMonster->creatureInfo->set_cur_hp(MONSTER_MAX_HP);
    newMonster->creatureInfo->set_damage(MONSTER_DAMAGE);

    newMonster->monsterInfo->set_monster_number(monster->monsterInfo->monster_number());

    return newMonster;
}