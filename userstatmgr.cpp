#include <bson/util/json.h>

#include "userstatmgr.h"
#include "loginserver.h"

UserStatMgr::UserStatMgr()
{
	
}

UserStatMgr::~UserStatMgr()
{
	
}

void UserStatMgr::init(LoginServer *p_proxy)
{
	m_pSproxy = p_proxy;
	m_pDbMgr  = m_pSproxy->m_pDbMgr;
	
	load();
}

void UserStatMgr::load()
{
	//从redis或mysql加载用户状态
	map<int, string> mapUid2SvrId;
	m_pSproxy->m_pUserSvrCache->getAllUidSvrIdInfo(mapUid2SvrId);
	for(map<int, string>::iterator it = mapUid2SvrId.begin(); it != mapUid2SvrId.end(); it++)
	{
		Json::Reader reader;
        Json::Value value;

        if (reader.parse(it->second, value))
        {
        	if(value.isMember(SVRIDKEY_PLATFORMSVRID))
			{
				UsrSvrInfo uPlatSvrInfo;
				uPlatSvrInfo.nSvrId = value[SVRIDKEY_PLATFORMSVRID].asInt();
				m_mapUid2PlatSvr[it->first] = uPlatSvrInfo;
        	}

			if(value.isMember(SVRIDKEY_GAMESVRID))
			{
				UsrSvrInfo uGameSvrInfo;
				uGameSvrInfo.nSvrId = value[SVRIDKEY_GAMESVRID].asInt();
				m_mapUid2GameSvr[it->first] = uGameSvrInfo;
			}
			m_mapUid2Stat[it->first] = USER_STAT_ONLINE;
        }
	}
}

int UserStatMgr::updateUserStat(int nUid, int nStat)
{
    base::Guard guard(m_lock);
    map<int, int>::iterator it = m_mapUid2Stat.find(nUid);
    if(it != m_mapUid2Stat.end())
    {
        it->second = nStat;
    }
	if(nStat == USER_STAT_OFFLINE)
	{
		m_mapUid2PlatSvr.erase(nUid);
		m_mapUid2GameSvr.erase(nUid);
		m_pSproxy->m_pUserSvrCache->removeUid2SvridInfo(nUid);
	}
    return ERROR_CODE::EC_Succeed;
}

int UserStatMgr::getUserStat(int nUid)
{
    int nStat = USER_STAT_OFFLINE;
    base::Guard guard(m_lock);
    map<int, int>::iterator it = m_mapUid2Stat.find(nUid);
    if(it != m_mapUid2Stat.end())
    {
        nStat = it->second;
    }
    return nStat;
}

int UserStatMgr::updateUserPlatSvr(int nUid, int nPlatSvrId)
{
	if(nPlatSvrId <= 0) 
	{
		return ERROR_CODE::EC_Succeed;
	}
    base::Guard guard(m_lock);
    map<int, UsrSvrInfo>::iterator it = m_mapUid2PlatSvr.find(nUid);
    if(it != m_mapUid2PlatSvr.end())
    {
        it->second.nSvrId = nPlatSvrId;
        it->second.nTimeStamp = time(NULL);
    }
    else
    {
        UsrSvrInfo svrInfo;
        svrInfo.nSvrId = nPlatSvrId;
        svrInfo.nTimeStamp = time(NULL);
        m_mapUid2PlatSvr[nUid] = svrInfo;
    }
	m_pSproxy->m_pUserSvrCache->setUser2PlatformSvrId(nUid, nPlatSvrId);
    return ERROR_CODE::EC_Succeed;
}

int UserStatMgr::updateUserGameSvr(int nUid, int nGameSvrId)
{
	if(nGameSvrId <= 0) 
	{
		return ERROR_CODE::EC_Succeed;
	}
    base::Guard guard(m_lock);
    map<int, UsrSvrInfo>::iterator it = m_mapUid2GameSvr.find(nUid);
    if(it != m_mapUid2GameSvr.end())
    {
        it->second.nSvrId = nGameSvrId;
        it->second.nTimeStamp = time(NULL);
    }
    else
    {
        UsrSvrInfo svrInfo;
        svrInfo.nSvrId = nGameSvrId;
        svrInfo.nTimeStamp = time(NULL);
        m_mapUid2GameSvr[nUid] = svrInfo;
    }
	m_pSproxy->m_pUserSvrCache->setUser2GameSvrId(nUid, nGameSvrId);
    return ERROR_CODE::EC_Succeed;
}

int UserStatMgr::getUserPlatSvr(int nUid)
{
    int nSvrId = -1;
    base::Guard guard(m_lock);
    map<int, UsrSvrInfo>::iterator it = m_mapUid2PlatSvr.find(nUid);
    if(it != m_mapUid2PlatSvr.end())
    {
        /*int nCurTime = time(NULL);
        if(nCurTime - it->second.nTimeStamp < STAT_OUT_DATE_TIME)
        {
            nSvrId = it->second.nSvrId;
        }
		else
		{
			m_mapUid2PlatSvr.erase(nUid);
			m_mapUid2GameSvr.erase(nUid);
			m_pDbMgr->removeUid2SvridInfo(nUid);
		}*/
		nSvrId = it->second.nSvrId;
    }
    
    return nSvrId;
}

int UserStatMgr::getUserGameSvr(int nUid)
{
    int nSvrId = -1;
    base::Guard guard(m_lock);
    map<int, UsrSvrInfo>::iterator it = m_mapUid2GameSvr.find(nUid);
    if(it != m_mapUid2GameSvr.end())
    {
        /*int nCurTime = time(NULL);
        if(nCurTime - it->second.nTimeStamp < STAT_OUT_DATE_TIME)
        {
            nSvrId = it->second.nSvrId;
        }
		else
		{
			m_mapUid2PlatSvr.erase(nUid);
			m_mapUid2GameSvr.erase(nUid);
			m_pDbMgr->removeUid2SvridInfo(nUid);
		}*/
		nSvrId = it->second.nSvrId;
    }
    
    return nSvrId;
}


