#pragma once

class ObjectUtils
{
public:
	static PlayerPtr CreatePlayer(GameSessionPtr session, string nickname);
	static MonsterPtr CreateMonster(MonsterPtr monster);

private:
	static atomic<int64> s_idGenerator;
};