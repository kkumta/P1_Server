#pragma once

class ObjectUtils
{
public:
	static PlayerPtr CreatePlayer(GameSessionPtr session, string nickname);
	static MonsterPtr CreateMonster(MonsterPtr monster);

private:
	static atomic<int64> s_idGenerator;
	enum HP : uint64
	{
		MONSTER_MAX_HP = 100,
		PLAYER_MAX_HP = 200,
	};
	enum Damage : uint64
	{
		MONSTER_DAMAGE = 10,
		PLAYER_DAMAGE = 20,
	};
};