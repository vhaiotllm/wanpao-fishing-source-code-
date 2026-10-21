#ifndef __USERSTAT_MGR_H__
#define __USERSTAT_MGR_H__

#include "nskernel/connection.h"
#include "base/lock_common.h"
#include "base/log.h"
#include "errorcode.h"

class LoginServer;
class DBMgr;

#define STAT_OUT_DATE_TIME  (5 * 60) //5分钟没断线重连，就算作过期

/*
* 记录用户状态，用户在哪个大厅服，在哪个游戏服
*/

struct UsrSvrInfo
{
    int nSvrId; //服务id
    int nTimeStamp; //最后更新时间，用作过期判断
    UsrSvrInfo()
	{
		nSvrId = 0;
		nTimeStamp = time(NULL);
	};
};

class UserStatMgr
{
public:
	UserStatMgr();
	~UserStatMgr();
	void init(LoginServer *p_proxy);
	void load();	   
    int updateUserStat(int nUid, int nStat);
    int getUserStat(int nUid);
    int updateUserPlatSvr(int nUid, int nPlatSvrId);
    int updateUserGameSvr(int nUid, int nGameSvrId);
    int getUserPlatSvr(int nUid);
    int getUserGameSvr(int nUid);
	

private:
	LoginServer* m_pSproxy;
	DBMgr* m_pDbMgr;	
	base::Lock m_lock;

	map<int, int> m_mapUid2Stat; //key是用户id，value是用户状态 1 在线 2 下线
    map<int, UsrSvrInfo> m_mapUid2PlatSvr; //key是用户id，用户所在平台信息
	map<int, UsrSvrInfo> m_mapUid2GameSvr; //key是用户id，用户所在游戏服信息
};

#endif

