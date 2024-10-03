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
    player->creatureInfo->set_object_id(newId);
    player->posInfo->set_object_id(newId);

    // GameData의 _player에 있는 정보로 플레이어 생성
    PlayerData playerData = GGameData->GetPlayer();

    player->creatureInfo->set_max_hp(playerData.GetHp());
    player->creatureInfo->set_cur_hp(playerData.GetHp());
    player->creatureInfo->set_damage(playerData.GetDamage());
    player->posInfo->set_x(playerData.GetX());
    player->posInfo->set_y(playerData.GetY());
    player->posInfo->set_z(playerData.GetZ());
    player->posInfo->set_yaw(playerData.GetYaw());

	player->session = session;
	session->player.store(player);

	return player;
}

MonsterPtr ObjectUtils::CreateMonster(uint64 monsterNumber)
{
    const int64_t newId = s_idGenerator.fetch_add(1);

    MonsterPtr newMonster = make_shared<Monster>();

    newMonster->objectInfo->set_object_id(newId);
    newMonster->creatureInfo->set_object_id(newId);
    newMonster->posInfo->set_object_id(newId);

    // GameData의 _monsters에 있는 정보로 몬스터 생성
    MonsterData monsterData = GGameData->GetMonster(monsterNumber);

    newMonster->monsterInfo->set_monster_number(monsterData.GetMonsterNumber());
    newMonster->objectInfo->set_nickname(monsterData.GetNickname());
    newMonster->creatureInfo->set_max_hp(monsterData.GetHp());
    newMonster->creatureInfo->set_cur_hp(monsterData.GetHp());
    newMonster->creatureInfo->set_damage(monsterData.GetDamage());
    newMonster->posInfo->set_x(monsterData.GetX());
    newMonster->posInfo->set_y(monsterData.GetY());
    newMonster->posInfo->set_z(monsterData.GetZ());
    newMonster->posInfo->set_yaw(monsterData.GetYaw());

    return newMonster;
}