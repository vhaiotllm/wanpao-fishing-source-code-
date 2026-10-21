#ifndef __BIGDATALOG_H__
#define __BIGDATALOG_H__

#include "nskernel/connection.h"
#include <bson.h>
#include "base/lock_common.h"
#include "base/log.h"
#include "loginserver.h"
#include "errorcode.h"
#include "dbmgr.h"
#include <json/json.h>

using namespace bson;

class LoginServer;
class DBMgr;

class BigDataLog
{
public:
	BigDataLog();
	~BigDataLog();
	void init(LoginServer *p_proxy);
	void load();	   //读取已注册的用户uid以及puid

	void recordUserRegLog(string strDevice, string strPuid, int nUid, string strChannel, string strVersion);
	void recordUserActLog(const int &nIsLogin, const int &nResult, string strDevice, int nLastLoginTime, int nUid, string strChannel, string strVersion, bool bIsNewUser, int nVip, int nLevel);
	void recordRoleRegLog(string strDevice, string strPuid, int nUid, string strChannel, string strVersion, string strNick);
	void recordRoleActLog(const int &nIsLogin, const int &nResult, string strDevice, int nLastLoginTime, int nUid, string strChannel, string strVersion, int nVip, int nLevel);



private:
	LoginServer* m_pSproxy;
	DBMgr* m_pDbMgr;	
	base::Lock m_lock;

};

#endif

