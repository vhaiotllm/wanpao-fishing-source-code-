#ifndef __REGIST_MGR_H__
#define __REGIST_MGR_H__

#include "nskernel/connection.h"
#include <bson.h>
#include "base/lock_common.h"
#include "base/log.h"
#include "loginserver.h"
#include "errorcode.h"
#include "dbmgr.h"
#include <json/json.h>
#include "proto_10100_wx_login.h"
#include "operatelog.h"


#define USER_ID_ORIGIN 214320 //随手填的一个数，避免从0开始，让玩家看出id分配的规律
#define ROBOT_ID_ORIGIN 100000 //机器人开始分配的id

using namespace bson;

class LoginServer;
class DBMgr;

class RegistMgr
{
public:
	RegistMgr();
	~RegistMgr();
	void init(LoginServer *p_proxy);
	void load();	   //读取已注册的用户uid以及puid
	bool checkAccountId(const string &puid);  //检测puid是否已注册
	bool checkUid(const int &uid);	

	int getUidByAccId(string &strAccId, string &strSdkId, bool &bIsMsdkUser); //根据puid获取uid
	string getAccIdByUid(const int &uid);

	int modifyUserRegistInfo(const int &uid, const string &puid, const string &oldname, const string &newname);
	bool telephoneIsValid(const string strTelephone);//手机号码是否已使用
	bool insertNewTelephone(const string strTelephone);
	bool removeTelephoneInfo(const string strTelephone);//解绑手机号码使用

    //10000协议，生成login的token，记录account_id ，account_token和 account_id，login_token
    string createToken(string strAccId, string strAccToken);
    int checkUserLogin(proto10login::C2SAccountLoginBody & c2sAccLogin, ProtocolMgr& packHeader, int &nUid);
    int dispatchFaceLook(int &nFaceId, int &isSys, int &nSex);
    int dispatchUid(const string &puid);
    bool checkLoginToken(string &strAccId, string &strSdkId, const string &strLoginToken);
    string getAccToken(const string &strAccId, const string &strSdkId);
    string getLoginToken(string strAccId);

	int checkWxLogin(proto10login::C2SWxLoginBody & c2s, proto10login::S2CWxLoginBody & s2c);
	string createWxToken(string strOpenId, string strWxLoginJson);

	void getUserMobile(proto10login::C2SAccountLoginBody & c2sAccLogin, string &strPhone, string &strSdkPhone);

	void updateNickFaceLook(bool bIsNewUser, int nUid,proto10login::C2SAccountLoginBody & c2sAccLogin, string &strNick, string &strFaceLook);

	int checkUserSdkLogin(proto10login::C2SAccountLoginBody & c2sAccLogin);

	int checkWhiteList(int nUid);

private:
	LoginServer* m_pSproxy;
	DBMgr* m_pDbMgr;	
	//RWLOCK m_lockUser;    // 进程启动加载所有的PUID
	base::Lock m_lock;
	int    m_nMaxUID;     //UID有序 分配最后一个

	map<string,int>  m_mapAccId2Uid;
	map<int, string> m_mapUid2AccId;
    map<string, int> m_mapPhone2Uid;
	map<string, set<int> > m_mapIp2UidSet; //key是注册ip的uid集合
	map<string, set<int> > m_mapDid2UidSet; //key是注册设备id的uid集合
	std::set<string> m_usedName; //已使用的姓名无序 查询使用
	std::set<string> m_bindTele; //已绑定的手机信息

    //map保存account_id ，account_token和 account_id，login_token，可记录到redis，做成无状态login
    map<string,string>  m_mapAccId2AccToken; //key是account_id，value是sdk平台的token
    map<string,string>  m_mapAccId2LoginToken; //key是account_id，value是loginserver收到10000协议生成的token

    //facelook id集，策划数据生成
    vector<int> m_vecFaceId; //分配新用户头像用，轮询
    int m_nCurVecFaceIdIndex;  //当前分配的头像数组下标，下次新用户就分配数组下一个id

	map<string, string> m_mapWxToken2LoginJson; //微信登录服生成的token对应微信登录后返回的json串信息

	int m_nLastLoadSysInfoTime; //上次load系统信息的时间点，隔一段时间去加载
};

#endif

