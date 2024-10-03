#pragma once

class ObjectUtils
{
public:
	static PlayerPtr CreatePlayer(GameSessionPtr session, string nickname);
	static MonsterPtr CreateMonster(uint64 monsterNumber);

private:
	static atomic<int64> s_idGenerator;
};