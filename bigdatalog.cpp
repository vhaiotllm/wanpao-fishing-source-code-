//#include <bson/util/json.h>

#include "bigdatalog.h"



BigDataLog::BigDataLog()
{
	//DORW_LOCK_INIT(m_lockUser);
}

BigDataLog::~BigDataLog()
{
	//DORW_LOCK_DESTROY(m_lockUser);
}

void BigDataLog::init(LoginServer *p_proxy)
{
	m_pSproxy = p_proxy;
	m_pDbMgr  = m_pSproxy->m_pDbMgr;
	
	load();
}

void BigDataLog::load()
{
	
}


void BigDataLog::recordUserRegLog(string strDevice, string strPuid, int nUid, string strChannel, string strVersion)
{	
	//每5分钟产生一个新文件
	struct timeval tv;
	gettimeofday(&tv,NULL);
	long lCurTime = tv.tv_sec*1000 + tv.tv_usec/1000;
	/*if (m_pSproxy->m_pDayLog[ACCOUNT_REG_LOG]->isOverTime(300))
	{
		m_pSproxy->m_pDayLog[ACCOUNT_REG_LOG]->setLogName(lCurTime);
	}*/
	int nPlatfrom = 5; //根据硬件信息判断是IOS还是安卓
	if (strDevice.find("android") != string::npos)
	{
		nPlatfrom = 1;
	}
	else if(strDevice.find("ios") != string::npos)
	{
		nPlatfrom = 2;
	}
	else if(strDevice.find("web") != string::npos)
	{
		nPlatfrom = 3;
	}
	//注册格式：biversion|appid|topic|trigger_time|platform|zone_id|uid|puid|name|channel|ip|IMEI|deviceid|last_level|client_version
	m_pSproxy->m_pDayLog[ACCOUNT_REG_LOG]->trace_reduce("1.3|1002|account_reg|%ld|%d|0|%s|%d|%s|%s|0",\
		lCurTime, nPlatfrom, strPuid.c_str(), nUid,  strChannel.c_str(), strVersion.c_str());
}

void BigDataLog::recordUserActLog(const int &nIsLogin, const int &nResult, string strDevice, int nLastLoginTime, int nUid, string strChannel, string strVersion, bool bIsNewUser, int nVip, int nLevel)
{
	//每5分钟产生一个新文件
	struct timeval tv;
	gettimeofday(&tv,NULL);
	long lCurTime = tv.tv_sec*1000 + tv.tv_usec/1000;
	/*if (m_pSproxy->m_pDayLog[ACCOUNT_ACT_LOG]->isOverTime(300))
	{
		m_pSproxy->m_pDayLog[ACCOUNT_ACT_LOG]->setLogName(lCurTime);
	}*/

	//int nPlatfrom = 2; //根据硬件信息判断是IOS还是安卓
	string strOnlineTime = "";
	string strAcc = "account_login";

	int nPlatfrom = 5; //根据硬件信息判断是IOS还是安卓
	if (strDevice.find("android") != string::npos)
	{
		nPlatfrom = 1;
	}
	else if(strDevice.find("ios") != string::npos)
	{
		nPlatfrom = 2;
	}
	else if(strDevice.find("web") != string::npos)
	{
		nPlatfrom = 3;
	}
	if (nIsLogin == 0)
	{
		strAcc = "account_logout";
		strOnlineTime = base::i2s(time(NULL) - nLastLoginTime);
	}
	//登录登出日志格式:biversion|appid|topic|trigger_time|platform|zone_id|
	m_pSproxy->m_pDayLog[ACCOUNT_ACT_LOG]->trace_reduce("1.3|1002|account_act|%ld|%d|0|%d|%s|%s|%s|%d|%s|2002|0|%d|%d|%d", \
		lCurTime, nPlatfrom, nUid, strChannel.c_str(), strAcc.c_str(), strOnlineTime.c_str(), nResult, strVersion.c_str(), bIsNewUser ? 0:1, nVip, nLevel); 
}

void BigDataLog::recordRoleRegLog(string strDevice, string strPuid, int nUid, string strChannel, string strVersion, string strNick)
{	
	//每5分钟产生一个新文件
	struct timeval tv;
	gettimeofday(&tv,NULL);
	long lCurTime = tv.tv_sec*1000 + tv.tv_usec/1000;
	/*if (m_pSproxy->m_pDayLog[ACCOUNT_REG_LOG]->isOverTime(300))
	{
		m_pSproxy->m_pDayLog[ACCOUNT_REG_LOG]->setLogName(lCurTime);
	}*/
	int nPlatfrom = 5; //根据硬件信息判断是IOS还是安卓
	if (strDevice.find("android") != string::npos)
	{
		nPlatfrom = 1;
	}
	else if(strDevice.find("ios") != string::npos)
	{
		nPlatfrom = 2;
	}
	else if(strDevice.find("web") != string::npos)
	{
		nPlatfrom = 3;
	}

	m_pSproxy->m_pDayLog[ROLE_REG_LOG]->trace_reduce("1.3|1002|role_reg|%ld|%d|0|%d|%s|%s|%s|%d|-1|%s",\
		lCurTime, nPlatfrom, nUid, strPuid.c_str(),strNick.c_str(),  strChannel.c_str(),nUid, strVersion.c_str());
}

void BigDataLog::recordRoleActLog(const int &nIsLogin, const int &nResult, string strDevice, int nLastLoginTime, int nUid, string strChannel, string strVersion, int nVip, int nLevel)
{
	//每5分钟产生一个新文件
	struct timeval tv;
	gettimeofday(&tv,NULL);
	long lCurTime = tv.tv_sec*1000 + tv.tv_usec/1000;
	/*if (m_pSproxy->m_pDayLog[ACCOUNT_ACT_LOG]->isOverTime(300))
	{
		m_pSproxy->m_pDayLog[ACCOUNT_ACT_LOG]->setLogName(lCurTime);
	}*/

	//int nPlatfrom = 2; //根据硬件信息判断是IOS还是安卓
	string strOnlineTime = "";
	string strAcc = "account_login";

	int nPlatfrom = 5; //根据硬件信息判断是IOS还是安卓
	if (strDevice.find("android") != string::npos)
	{
		nPlatfrom = 1;
	}
	else if(strDevice.find("ios") != string::npos)
	{
		nPlatfrom = 2;
	}
	else if(strDevice.find("web") != string::npos)
	{
		nPlatfrom = 3;
	}
	if (nIsLogin == 0)
	{
		strAcc = "account_logout";
		strOnlineTime = base::i2s(time(NULL) - nLastLoginTime);
	}

	m_pSproxy->m_pDayLog[ROLE_ACT_LOG]->trace_reduce("1.3|1002|role_act|%ld|%d|0|%d|%s|%d|%d|%d|%s|%s|%d|%s", \
		lCurTime, nPlatfrom, nUid, strChannel.c_str(),nUid,nVip, nLevel,strAcc.c_str(), strOnlineTime.c_str(), nResult, strVersion.c_str()); 
}


