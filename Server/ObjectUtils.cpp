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

	player->session = session;
	session->player.store(player);

	return player;
}

MonsterPtr ObjectUtils::CreateMonster(MonsterPtr monster)
{
    const int64_t newId = s_idGenerator.fetch_add(1);

    MonsterPtr newMonster = make_shared<Monster>();

    // monster의 objectInfo와 posInfo를 newMonster로 복사
    if (monster->objectInfo && newMonster->objectInfo)
    {
        newMonster->objectInfo->CopyFrom(*monster->objectInfo);
    }
    newMonster->objectInfo->set_object_id(newId);
    newMonster->posInfo->set_object_id(newId);

    return newMonster;
}