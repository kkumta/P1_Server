#include "pch.h"
#include "DBJobQueue.h"
#include "DBConnectionPool.h"
#include "DBConnection.h"
#include "GameSession.h"

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

	// ID, PW가 비어있는지 확인
	if (nickname.empty() || password.empty())
	{
		joinPkt.set_success(false);
		SEND_PACKET(joinPkt);
		return false;
	}

	// 이미 존재하는 ID인지 확인
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		if (dbConn == nullptr)
		{
			CRASH("DB Connection No Exists")
				return false;
		}

		// 기존에 바인딩 된 정보 날림
		dbConn->Unbind();

		// 넘길 인자 바인딩
		SQLLEN nicknameLen = nickname.length();
		ASSERT_CRASH(dbConn->BindParam(1, SQL_C_CHAR, SQL_VARCHAR, nicknameLen, const_cast<char*>(nickname.c_str()), &nicknameLen));

		int32 exists = 0;
		SQLLEN existsLen = 0;
		ASSERT_CRASH(dbConn->BindCol(1, SQL_C_LONG, sizeof(exists), &exists, &existsLen));

		// SQL 실행
		ASSERT_CRASH(dbConn->Execute(L"SELECT COUNT(*) FROM [dbo].[users] WHERE nickname = (?)"));

		// 결과 확인
		dbConn->Fetch();

		GDBConnectionPool->Push(dbConn);

		if (exists > 0)
		{
			joinPkt.set_success(false);
			SEND_PACKET(joinPkt);
			return false;
		}
	}

	// DB에 새로운 회원 정보 추가
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		if (dbConn == nullptr)
		{
			CRASH("DB Connection No Exists")
				return false;
		}

		// 기존에 바인딩 된 정보 날림
		dbConn->Unbind();

		// 넘길 인자 바인딩
		SQLLEN nicknameLen = nickname.length();
		SQLLEN passwordLen = password.length();
		ASSERT_CRASH(dbConn->BindParam(1, SQL_C_CHAR, SQL_VARCHAR, nicknameLen, const_cast<char*>(nickname.c_str()), &nicknameLen));
		ASSERT_CRASH(dbConn->BindParam(2, SQL_C_CHAR, SQL_VARCHAR, passwordLen, const_cast<char*>(password.c_str()), &passwordLen));

		// SQL 실행 (prepared statement recommended for security)
		ASSERT_CRASH(dbConn->Execute(L"INSERT INTO [dbo].[users] (nickname, password) VALUES (?, ?)"));
		GDBConnectionPool->Push(dbConn);
	}

	// 클라이언트에 패킷 보내기
	joinPkt.set_success(true);
	SEND_PACKET(joinPkt);

	return true;
}

bool DBJobQueue::HandleLogin(PacketSessionPtr session, Protocol::C_LOGIN pkt)
{
	Protocol::S_LOGIN loginPkt;

	string nickname = pkt.logininfo().nickname();
	string password = pkt.logininfo().password();

	// ID, PW가 비어있는지 확인
	if (nickname.empty() || password.empty())
	{
		loginPkt.set_success(false);
		SEND_PACKET(loginPkt);
		return false;
	}

	// DB에 저장된 닉네임, 비밀번호인지 확인
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		if (dbConn == nullptr)
		{
			CRASH("DB Connection No Exists")
				return false;
		}

		// 기존에 바인딩 된 정보 날림
		dbConn->Unbind();

		// 넘길 인자 바인딩
		SQLLEN nicknameLen = nickname.length();
		ASSERT_CRASH(dbConn->BindParam(1, SQL_C_CHAR, SQL_VARCHAR, nicknameLen, const_cast<char*>(nickname.c_str()), &nicknameLen));

		vector<char> outPasswordBuffer(256);
		SQLLEN outPasswordLen = 0;
		ASSERT_CRASH(dbConn->BindCol(1, SQL_C_CHAR, outPasswordBuffer.size(), outPasswordBuffer.data(), &outPasswordLen));

		// SQL 실행
		ASSERT_CRASH(dbConn->Execute(L"SELECT password FROM [dbo].[users] WHERE nickname = (?)"));

		// 결과 확인
		dbConn->Fetch();

		string outPassword(outPasswordBuffer.data(), outPasswordLen);

		GDBConnectionPool->Push(dbConn);

		if (outPassword != password)
		{
			loginPkt.set_success(false);
			SEND_PACKET(loginPkt);
			return false;
		}
	}

	// 클라이언트에 패킷 보내기
	loginPkt.set_success(true);
	loginPkt.set_nickname(nickname);
	SEND_PACKET(loginPkt);

	return true;
}