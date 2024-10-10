#include "pch.h"
#include "DBJobQueue.h"
#include "DBConnectionPool.h"
#include "DBConnection.h"
#include "GameSession.h"
#include "Logger.h"

DBJobQueuePtr GDBJobQueue = make_shared<DBJobQueue>();

DBJobQueue::DBJobQueue() : JobQueue(THREAD_TYPE::DB)
{
}

DBJobQueue::~DBJobQueue()
{
}

bool DBJobQueue::HandleJoin(PacketSessionPtr session, Protocol::C_JOIN pkt)
{
	Protocol::S_JOIN joinPkt;

	string nickname = pkt.joininfo().nickname();
	string password = pkt.joininfo().password();

	// ID, PW�� ����ִ��� Ȯ��
	if (nickname.empty() || password.empty())
	{
		joinPkt.set_success(false);
		SEND_PACKET(joinPkt);
		GLogger.logJoin(nickname, false, "ID/password is empty");
		return false;
	}

	// �̹� �����ϴ� ID���� Ȯ��
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		if (dbConn == nullptr)
		{
			GLogger.logJoin(nickname, false, "DB Connection no exists");
			CRASH("DB Connection No Exists")
				return false;
		}

		// ������ ���ε� �� ���� ����
		dbConn->Unbind();

		// �ѱ� ���� ���ε�
		SQLLEN nicknameLen = nickname.length();
		ASSERT_CRASH(dbConn->BindParam(1, SQL_C_CHAR, SQL_VARCHAR, nicknameLen, const_cast<char*>(nickname.c_str()), &nicknameLen));

		int32 exists = 0;
		SQLLEN existsLen = 0;
		ASSERT_CRASH(dbConn->BindCol(1, SQL_C_LONG, sizeof(exists), &exists, &existsLen));

		// SQL ����
		ASSERT_CRASH(dbConn->Execute(L"SELECT COUNT(*) FROM [dbo].[users] WHERE nickname = (?)"));

		// ��� Ȯ��
		dbConn->Fetch();

		GDBConnectionPool->Push(dbConn);

		if (exists > 0)
		{
			joinPkt.set_success(false);
			SEND_PACKET(joinPkt);
			GLogger.logJoin(nickname, false, "Nickname already exists");
			return false;
		}
	}

	// DB�� ���ο� ȸ�� ���� �߰�
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		if (dbConn == nullptr)
		{
			GLogger.logJoin(nickname, false, "DB Connection no exists");
			CRASH("DB Connection No Exists")
				return false;
		}

		// ������ ���ε� �� ���� ����
		dbConn->Unbind();

		// �ѱ� ���� ���ε�
		SQLLEN nicknameLen = nickname.length();
		SQLLEN passwordLen = password.length();
		ASSERT_CRASH(dbConn->BindParam(1, SQL_C_CHAR, SQL_VARCHAR, nicknameLen, const_cast<char*>(nickname.c_str()), &nicknameLen));
		ASSERT_CRASH(dbConn->BindParam(2, SQL_C_CHAR, SQL_VARCHAR, passwordLen, const_cast<char*>(password.c_str()), &passwordLen));

		// SQL ���� (prepared statement recommended for security)
		ASSERT_CRASH(dbConn->Execute(L"INSERT INTO [dbo].[users] (nickname, password) VALUES (?, ?)"));
		GDBConnectionPool->Push(dbConn);
	}

	// Ŭ���̾�Ʈ�� ��Ŷ ������
	joinPkt.set_success(true);
	SEND_PACKET(joinPkt);
	GLogger.logJoin(nickname, true);

	return true;
}

bool DBJobQueue::HandleLogin(PacketSessionPtr session, Protocol::C_LOGIN pkt)
{
	Protocol::S_LOGIN loginPkt;

	string nickname = pkt.logininfo().nickname();
	string password = pkt.logininfo().password();

	// ID, PW�� ����ִ��� Ȯ��
	if (nickname.empty() || password.empty())
	{
		loginPkt.set_success(false);
		SEND_PACKET(loginPkt);
		GLogger.logLogin(nickname, false, "Nickname/password is empty");
		return false;
	}

	// DB�� ����� �г���, ��й�ȣ���� Ȯ��
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		if (dbConn == nullptr)
		{
			GLogger.logLogin(nickname, false, "DB Connection no exists");
			CRASH("DB Connection No Exists")
				return false;
		}

		// ������ ���ε� �� ���� ����
		dbConn->Unbind();

		// �ѱ� ���� ���ε�
		SQLLEN nicknameLen = nickname.length();
		ASSERT_CRASH(dbConn->BindParam(1, SQL_C_CHAR, SQL_VARCHAR, nicknameLen, const_cast<char*>(nickname.c_str()), &nicknameLen));

		vector<char> outPasswordBuffer(256);
		SQLLEN outPasswordLen = 0;
		ASSERT_CRASH(dbConn->BindCol(1, SQL_C_CHAR, outPasswordBuffer.size(), outPasswordBuffer.data(), &outPasswordLen));

		// SQL ����
		ASSERT_CRASH(dbConn->Execute(L"SELECT password FROM [dbo].[users] WHERE nickname = (?)"));

		// ��� Ȯ��
		dbConn->Fetch();

		string outPassword(outPasswordBuffer.data(), outPasswordLen);

		GDBConnectionPool->Push(dbConn);

		if (outPassword != password)
		{
			loginPkt.set_success(false);
			SEND_PACKET(loginPkt);

			GLogger.logLogin(nickname, false, "Password is not matched");
			return false;
		}
	}

	// Ŭ���̾�Ʈ�� ��Ŷ ������
	loginPkt.set_success(true);
	loginPkt.set_nickname(nickname);
	SEND_PACKET(loginPkt);

	GLogger.logLogin(nickname, true);
	return true;
}

void DBJobQueue::HandleIncreaseExp(const string nickname, const uint64 rewardExp)
{
	// DB�� ����� �г����� �����ϴ��� Ȯ���ϰ�, EXP�� ��ȸ �� ����
	DBConnection* dbConn = GDBConnectionPool->Pop();
	if (dbConn == nullptr)
	{
		GLogger.logIncreaseExp("Player", "nickname", rewardExp, false, "DB Connection no exists");
		CRASH("DB Connection No Exists");
		return;
	}

	// ������ ���ε��� ���� ����
	dbConn->Unbind();

	// �ѱ� ���� ���ε� (�г���)
	SQLLEN nicknameLen = nickname.length();
	ASSERT_CRASH(dbConn->BindParam(1, SQL_C_CHAR, SQL_VARCHAR, nicknameLen, const_cast<char*>(nickname.c_str()), &nicknameLen));

	// ������� ���� ���� ���� (exp)
	uint64 currentExp = 0;
	SQLLEN currentExpLen = sizeof(currentExp);
	ASSERT_CRASH(dbConn->BindCol(1, SQL_C_UBIGINT, sizeof(currentExp), &currentExp, &currentExpLen));

	// SQL ���� (�г������� exp ��ȸ)
	ASSERT_CRASH(dbConn->Execute(L"SELECT exp FROM [dbo].[users] WHERE nickname = (?)"));

	// ��� Ȯ��
	dbConn->Fetch();

	// ��ȸ�� ���� rewardExp ����
	uint64 newExp = currentExp + rewardExp;

	// Unbind �� ���ο� ���� ������Ʈ
	dbConn->Unbind();
	ASSERT_CRASH(dbConn->BindParam(1, SQL_C_UBIGINT, SQL_BIGINT, sizeof(newExp), &newExp, &currentExpLen));
	ASSERT_CRASH(dbConn->BindParam(2, SQL_C_CHAR, SQL_VARCHAR, nicknameLen, const_cast<char*>(nickname.c_str()), &nicknameLen));

	// SQL ���� (exp ������Ʈ)
	ASSERT_CRASH(dbConn->Execute(L"UPDATE [dbo].[users] SET exp = (?) WHERE nickname = (?)"));

	// DB ������ �ٽ� Ǯ�� ��ȯ
	GDBConnectionPool->Push(dbConn);

	GLogger.logIncreaseExp("Player", nickname, rewardExp, true);
}