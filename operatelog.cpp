//#include <bson/util/json.h>

#include "operatelog.h"

const int OP_LOG_APPID = 1001; //运营中心后台定义的捕鱼appid
const int PLATFORM_ROOMID = 1; //大厅房间id
const string OP_LOG_VISION = "1.0"; //日志版本号

OperateLog::OperateLog()
{
	//DORW_LOCK_INIT(m_lockUser);
}

OperateLog::~OperateLog()
{
	//DORW_LOCK_DESTROY(m_lockUser);
}

void OperateLog::init(LoginServer *p_proxy)
{
	m_pSproxy = p_proxy;
	m_pDbMgr  = m_pSproxy->m_pDbMgr;
	
	load();
}

void OperateLog::load()
{
	
}

//基础日志数据
string OperateLog::getBaseParam(string strLogTopic, string strDevice, string strPuid, int nUid, string strChannel, string strVersion, int nGameId)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	long lCurTime = tv.tv_sec*1000 + tv.tv_usec/1000;
	/*if (m_pSproxy->m_pDayLog[ACCOUNT_REG_LOG]->isOverTime(300))
	{
		m_pSproxy->m_pDayLog[ACCOUNT_REG_LOG]->setLogName(lCurTime);
	}*/
	int nPlatfrom = 1; //根据硬件信息判断是IOS还是安卓
	if (strDevice.find("android") != string::npos)
	{
		nPlatfrom = 2;
	}
	else if(strDevice.find("ios") != string::npos)
	{
		nPlatfrom = 1;
	}
	else if(strDevice.find("web") != string::npos)
	{
		nPlatfrom = 3;
	}

	char szParam[512] = {0};
	sprintf(szParam, "%s|%s|%d|%s|%ld|%d|0|%s|%d|%s|%d|%s|%d",\
		base::t2s(time(NULL)).c_str(),OP_LOG_VISION.c_str(),OP_LOG_APPID,strLogTopic.c_str(),lCurTime, nPlatfrom, strVersion.c_str(),  nGameId, strChannel.c_str(), nUid, strPuid.c_str(), nUid);
	return szParam;
}


void OperateLog::recordUserRegLog(string strDevice, string strPuid, int nUid, string strChannel, string strVersion, string strUserAccount, string strDid, string strDeviceType, string strIp)
{	
	string strBaseParam = getBaseParam("account_reg_log", strDevice,strPuid, nUid,strChannel,strVersion,0);
	//注册
	m_pSproxy->m_pOperateLog[OP_LOG_ACCOUNT_REG_LOG]->trace_reduce("%s|%s|%s|%s|%s",strBaseParam.c_str(), strUserAccount.c_str(), strDid.c_str(), strDeviceType.c_str(), strIp.c_str());
}

void OperateLog::recordUserActLog(string strPuid, string strDevice, string strDid, string strIp, int operationCode, int onlineTime, int nUid, string strChannel, string strVersion)
{
	string strBaseParam = getBaseParam("account_act_log", strDevice,strPuid, nUid,strChannel,strVersion,0);
	//登录登出
	m_pSproxy->m_pOperateLog[OP_LOG_ACCOUNT_ACT_LOG]->trace_reduce("%s|%d|%s|%s|%d|%d",strBaseParam.c_str(), PLATFORM_ROOMID, strDid.c_str(), strIp.c_str(), operationCode, onlineTime);
}





