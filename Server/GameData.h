#pragma once

class MonsterData
{
public:
	MonsterData(uint64 monsterNumber, string nickname, uint64 hp, uint64 damage, float x, float y, float z, float yaw)
		: _monsterNumber(monsterNumber), _nickname(nickname), _hp(hp), _damage(damage), _x(x), _y(y), _z(z), _yaw(yaw)
	{
	}

	uint64 GetMonsterNumber() const { return _monsterNumber; }
	string GetNickname() const { return _nickname; }
	uint64 GetHp() const { return _hp; }
	uint64 GetDamage() const { return _damage; }
	float GetX() const { return _x; }
	float GetY() const { return _y; }
	float GetZ() const { return _z; }
	float GetYaw() const { return _yaw; }

private:
	uint64 _monsterNumber;
	string _nickname;
	uint64 _hp;
	uint64 _damage;
	float _x;
	float _y;
	float _z;
	float _yaw;
};

class PlayerData
{
public:
	PlayerData()
		: _hp(0), _damage(0), _x(0.0f), _y(0.0f), _z(0.0f), _yaw(0.0f)
	{
	}
	PlayerData(uint64 hp, uint64 damage, float x, float y, float z, float yaw)
		: _hp(hp), _damage(damage), _x(x), _y(y), _z(z), _yaw(yaw)
	{
	}
	
	uint64 GetHp() const { return _hp; }
	uint64 GetDamage() const { return _damage; }
	float GetX() const { return _x; }
	float GetY() const { return _y; }
	float GetZ() const { return _z; }
	float GetYaw() const { return _yaw; }

private:
	uint64 _hp;
	uint64 _damage;
	float _x;
	float _y;
	float _z;
	float _yaw;
};

class GameData
{
public:
	GameData() {}
	void LoadData(const string& file_path);
	const vector<MonsterData>& GetMonsters() const { return _monsters; }
	const MonsterData& GetMonster(uint64 monsterNumber) const;
	const PlayerData& GetPlayer() const { return _player; }

private:
	vector<MonsterData> _monsters;
	PlayerData _player;
};

extern shared_ptr<GameData> GGameData;