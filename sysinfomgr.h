#ifndef __SYS_INFO_MGR_H__
#define __SYS_INFO_MGR_H__

#include "nskernel/connection.h"
#include <bson.h>
#include "base/lock_common.h"
#include "base/log.h"
#include "errorcode.h"
#define ORDER_ID_ORIGIN 100000
#define MAIL_ID_ORIGIN 100000
#define ECODE_ORIGIN 1000


using namespace bson;

class LoginServer;
class DBMgr;

/*
	id管理分配，因为以后要分服，所以id都是跟着服务id，拼接：svrid+xxid
	记录系统信息
*/
class SysInfoMgr
{
public:
	SysInfoMgr();
	~SysInfoMgr();
	void init(LoginServer *p_proxy);
	void load();	   //读取数据库里最大的id
	
	void loadSvrStopTime();
	int updateSvrStopTime(int nTime);
	int getSvrStopTime();
	void loadWhiteListOpen();
	int updateWhiteListOpen(int nOpen);
	int getWhiteListOpen();
	void loadWhiteList();
	string updateWhiteList(string strWhiteList);
	string getWhiteList();

private:
	LoginServer* m_pSproxy;
	DBMgr* m_pDbMgr;	

	int		m_nSvrStopTime; //服务器停服时间戳，gm后台配
	int		m_nWhiteListOpen; //白名单是否开启 1开启 0不开
	string  m_strWhiteList; //白名单
	

	//标志位
	bool m_bIsSysNewSvrStopTime;
	bool m_bIsSysNewWhiteListOpen;
	bool m_bIsSysNewWhiteList;

};

#endif


