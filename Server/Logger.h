#pragma once
#include "pch.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

class Logger
{
public:
	Logger()
	{
		// Create a logger that logs to both console and file
		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs.txt", true);
		auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
		logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{ file_sink, console_sink });
		spdlog::register_logger(logger);
	}

	void logJoin(const std::string& nickname, bool success, const std::string& reason = "")
	{
		if (success)
		{
			logger->info("Action: Join | Nickname: {} | Result: Success", nickname);
		}
		else
		{
			logger->info("Action: Join | Nickname: {} | Result: Fail | Reason: {}", nickname, reason);
		}
	}

	void logLogin(const std::string& nickname, bool success, const std::string& reason = "")
	{
		if (success)
		{
			logger->info("Action: Login | Nickname: {} | Result: Success", nickname);
		}
		else
		{
			logger->info("Action: Login | Nickname: {} | Result: Fail | Reason: {}", nickname, reason);
		}
	}

	void logSpawn(const std::string& entityType, uint64 objectId, const std::string& nickname,
		float x, float y, float z, float yaw, bool success, const std::string& reason = "")
	{
		if (success)
		{
			logger->info("Action: Spawn | Entity: {} (ObjectId: {}) | Nickname: {} | Position: ({}, {}, {}, Yaw: {}) | Result: Success",
				entityType, objectId, nickname, x, y, z, yaw);
		}
		else
		{
			logger->info("Action: Spawn | Entity: {} (ObjectId: {}) | Nickname: {} | Position: ({}, {}, {}, Yaw: {}) | Result: Fail | Reason: {}",
				entityType, objectId, nickname, x, y, z, yaw, reason);
		}
	}

	void logMove(const std::string& entityType, uint64 objectId, const std::string& nickname,
		float x, float y, float z, float yaw, bool success, const std::string& reason = "")
	{
		if (success)
		{
			logger->info("Action: Move | Entity: {} (ObjectId: {}) | Nickname: {} | Position: ({}, {}, {}, Yaw: {}) | Result: Success",
				entityType, objectId, nickname, x, y, z, yaw);
		}
		else
		{
			logger->info("Action: Move | Entity: {} (ObjectId: {}) | Nickname: {} | Position: ({}, {}, {}, Yaw: {}) | Result: Fail | Reason: {}",
				entityType, objectId, nickname, x, y, z, yaw, reason);
		}
	}

	void logAttack(const std::string& attackerEntityType, uint64 attackerObjectId, const std::string& attackerNickname,
		const std::string& victimEntityType, uint64 victimObjectId, const std::string& victimNickname,
		uint64 damage, float x, float y, float z, float yaw, bool success, const std::string& reason = "")
	{
		if (success)
		{
			logger->info("Action: Attack | Attacker: {} ({}, ObjectId: {}) | Victim: {} ({}, ObjectId: {}) | Position: ({}, {}, {}, Yaw: {}) | Damage: {} | Result: Success",
				attackerEntityType, attackerNickname, attackerObjectId, victimEntityType, victimNickname, victimObjectId, x, y, z, yaw, damage);
		}
		else
		{
			logger->info("Action: Attack | Attacker: {} ({}, ObjectId: {}) | Victim: {} ({}, ObjectId: {}) | Position: ({}, {}, {}, Yaw: {}) | Damage: {} | Result: Fail | Reason: {}",
				attackerEntityType, attackerNickname, attackerObjectId, victimEntityType, victimNickname, victimObjectId, x, y, z, yaw, damage, reason);
		}
	}

	void logHit(const std::string& attackerEntityType, uint64 attackerObjectId, const std::string& attackerNickname,
		const std::string& victimEntityType, uint64 victimObjectId, const std::string& victimNickname,
		uint64 hp, uint64 damage, float x, float y, float z, float yaw, bool success, const std::string& reason = "")
	{
		if (success)
		{
			logger->info("Action: Hit | Attacker: {} ({}, ObjectId: {}) | Victim: {} (ObjectId: {}, Nickname: {}) | Position: ({}, {}, {}, Yaw: {}) | HP: {} | Damage: {} | Result: Success",
				attackerEntityType, attackerNickname, attackerObjectId, victimEntityType, victimObjectId, victimNickname, x, y, z, yaw, hp, damage);
		}
		else
		{
			logger->info("Action: Hit | Attacker: {} ({}, ObjectId: {}) | Victim: {} (ObjectId: {}, Nickname: {}) | Position: ({}, {}, {}, Yaw: {}) | HP: {} | Damage: {} | Result: Fail | Reason: {}",
				attackerEntityType, attackerNickname, attackerObjectId, victimEntityType, victimObjectId, victimNickname, x, y, z, yaw, hp, damage, reason);
		}
	}

	void logDeath(const std::string& entityType, uint64 objectId, const std::string& nickname, bool success, const std::string& reason = "")
	{
		if (success)
		{
			logger->info("Action: Death | Entity: {} (ObjectId: {}, Nickname: {}) | Result: Success",
				entityType, objectId, nickname);
		}
		else
		{
			logger->info("Action: Death | Entity: {} (ObjectId: {}, Nickname: {}) | Result: Fail | Reason: {}",
				entityType, objectId, nickname, reason);
		}
	}


	void logIncreaseExp(const std::string& entityType, const std::string& nickname, uint64 rewardExp, bool success, const std::string& reason = "")
	{
		if (success)
		{
			logger->info("Action: IncreaseExp | Entity: {} | Nickname: {} | RewardExp: {} | Result: Success",
				entityType, nickname, rewardExp);
		}
		else
		{
			logger->info("Action: IncreaseExp | Entity: {} | Nickname: {} | RewardExp: {} | Result: Fail | Reason: {}",
				entityType, nickname, rewardExp, reason);
		}
	}


private:
	std::shared_ptr<spdlog::logger> logger;
};

extern Logger GLogger;