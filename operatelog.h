#ifndef __OPERATELOG_H__
#define __OPERATELOG_H__

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

class OperateLog
{
public:
	OperateLog();
	~OperateLog();
	void init(LoginServer *p_proxy);
	void load();	   

	string getBaseParam(string strLogTopic, string strDevice, string strPuid, int nUid, string strChannel, string strVersion, int nGameId);
	void recordUserRegLog(string strDevice, string strPuid, int nUid, string strChannel, string strVersion, string strUserAccount, string strDid, string strDeviceType, string strIp);
	void recordUserActLog(string strPuid, string strDevice, string strDid, string strIp, int operationCode, int onlineTime, int nUid, string strChannel, string strVersion);



private:
	LoginServer* m_pSproxy;
	DBMgr* m_pDbMgr;	
	base::Lock m_lock;

};

#endif


