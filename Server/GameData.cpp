#include "pch.h"
#include "GameData.h"
#include <fstream>
#include "json.hpp"

//GameDataPtr GGameData = make_shared<GameData>();
shared_ptr<GameData> GGameData = make_shared<GameData>();
using json = nlohmann::json;

// JSON 데이터 파일을 읽어 객체로 변환하는 함수
void GameData::LoadData(const std::string& file_path)
{
	std::ifstream file(file_path);
	json data;

	if (!file.is_open())
	{
		std::cerr << "Error: Could not open file " << file_path << std::endl;
		return;
	}

	try
	{
		file >> data;
	}
	catch (const json::parse_error& e)
	{
		std::cerr << "JSON Parse Error: " << e.what() << std::endl;
		std::cerr << "Error ID: " << e.id << std::endl;
		std::cerr << "Byte Position: " << e.byte << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}


	// MonsterData 파싱
	for (const auto& item : data["monsters"])
	{
		MonsterData monster(item["monsterNumber"], item["nickname"], item["hp"], item["damage"], item["x"], item["y"], item["z"], item["yaw"]);
		_monsters.push_back(monster);
	}

	// PlayerData 파싱
	{
		auto item = data["player"];
		_player = PlayerData(item["hp"], item["damage"], item["x"], item["y"], item["z"], item["yaw"]);
	}
}

const MonsterData& GameData::GetMonster(uint64 monsterNumber) const
{
	for (const auto& monster : _monsters)
	{
		if (monster.GetMonsterNumber() == monsterNumber)
			return monster;
	}

	throw std::runtime_error("Monster not found");
}