//#include <bson/util/json.h>

#include "registmgr.h"

#include "backstagemgr.h"
#include "config_mgr.h"
#include "bigdatalog.h"
#include "sysinfomgr.h"
#include "common_func.h"

RegistMgr::RegistMgr()
{
	//DORW_LOCK_INIT(m_lockUser);
}

RegistMgr::~RegistMgr()
{
	//DORW_LOCK_DESTROY(m_lockUser);
}

void RegistMgr::init(LoginServer *p_proxy)
{
	m_pSproxy = p_proxy;
	m_pDbMgr  = m_pSproxy->m_pDbMgr;
    m_nCurVecFaceIdIndex = 0;
	
	load();
}

void RegistMgr::load()
{
	m_mapAccId2Uid.clear();
	m_mapUid2AccId.clear();
	m_usedName.clear();
	m_bindTele.clear();
	m_nMaxUID = USER_ID_ORIGIN;
	m_nLastLoadSysInfoTime = time(NULL);

	string strTbl = TBL_PLAYER;
	string strSql = "";
	for (int i = 0; i < 10; i++)
	{
		string strDbTbl = strTbl + base::i2s(i);
		strSql = "select uid, account_id, nick, phone from " + strDbTbl + " order by uid;";
		base::MySqlData data = m_pDbMgr->query_sql(strSql.c_str());
		int rowNum = data.num_rows();
		for (int j = 0; j < rowNum; j++)
		{
			int uid = base::s2i(data[j]["uid"]);
			string puid = data[j]["account_id"];
			string name = data[j]["nick"];
			string phone = data[j]["phone"];

			m_mapUid2AccId[uid]  = puid;
			m_mapAccId2Uid[puid] = uid;
			if (phone != "")
			{
                m_mapPhone2Uid[phone] = uid;
			}
			
			if (uid > m_nMaxUID) m_nMaxUID = uid;
		//	m_pSproxy->m_pRollLog->debug("RegistMgr::loadPlayerInfo(uid:%d,puid:%s,name:%s, tele:%s)", uid, puid.c_str(), name.c_str(), tele.c_str());
		} 
	}
	
	strTbl = TBL_PLAYER_LOGININFO;
	strSql = "";
	for (int i = 0; i < 10; i++)
	{
		string strDbTbl = strTbl + base::i2s(i);
		strSql = "select uid, reg_ip, reg_did from " + strDbTbl + " order by uid;";
		base::MySqlData data = m_pDbMgr->query_sql(strSql.c_str());
		int rowNum = data.num_rows();
		for (int j = 0; j < rowNum; j++)
		{
			int uid = base::s2i(data[j]["uid"]);
			string strRegIp = data[j]["reg_ip"];
			string strRegDid = data[j]["reg_did"];

			if(strRegIp != "")
			{
				if(m_mapIp2UidSet.find(strRegIp) != m_mapIp2UidSet.end())
				{
					m_mapIp2UidSet[strRegIp].insert(uid);
				}
				else
				{
					set<int> uidSet;
					uidSet.insert(uid);
					m_mapIp2UidSet[strRegIp] = uidSet;
				}
			}
			if(strRegDid != "")
			{
				if(m_mapDid2UidSet.find(strRegDid) != m_mapDid2UidSet.end())
				{
					m_mapDid2UidSet[strRegDid].insert(uid);
				}
				else
				{
					set<int> uidSet;
					uidSet.insert(uid);
					m_mapDid2UidSet[strRegDid] = uidSet;
				}
			}
		} 
	}

    /*QueryParam param;
	param.tableName = "tbl_facelook";
	param.keyFieldName = "id";

    DataVector vecRecord;
    m_pDbMgr->get(param, vecRecord);
    for(int i = 0; i < vecRecord.size(); i++)
    {
		msgpack::unpacked unpack;	
	  	msgpack::unpack(&unpack, vecRecord[i].data(), vecRecord[i].size());  
	  	msgpack::object  obj1 = unpack.get();
        stringstream ss;
        ss << obj1;
        m_pSproxy->m_pRollLog->debug("RegistMgr::load(ss:%s)", ss.str().c_str());

        Json::Reader reader;
        Json::Value value;

        if (reader.parse(ss.str(), value))
        {
            int nFaceId = value["id"].asInt();
            m_vecFaceId.push_back(nFaceId);
        }
    }*/
    /*map<int, TblConfFacelook> mapId2TblConfFacelook = m_pSproxy->m_pDc->m_pFacelookConf->get_all_TblConfFacelook();
	for(map<int, TblConfFacelook>::iterator it = mapId2TblConfFacelook.begin(); it != mapId2TblConfFacelook.end(); it++)
	{
		int nFaceId = it->first;
        m_vecFaceId.push_back(nFaceId);
	}*/

	const std::map<int, CsvConfig::Facelook> facelookMap = CsvConfigMgr::getInstance().getFacelookMap();
	for(std::map<int, CsvConfig::Facelook>::const_iterator it = facelookMap.begin(); it != facelookMap.end(); it++)
	{
		int nFaceId = it->first;
        m_vecFaceId.push_back(nFaceId);
	}
	
	m_pSproxy->m_pRollLog->debug("RegistMgr::load user max id:%d", m_nMaxUID);
}

bool RegistMgr::telephoneIsValid(const string strTelephone)
{
	bool bIsValid = true;
	set<string>::iterator iter = m_bindTele.find(strTelephone);
	if (iter != m_bindTele.end())//已绑定的手机不能再绑定
	{
		bIsValid = false;
	}
	return bIsValid;
}

bool RegistMgr::insertNewTelephone(const string strTelephone)
{
	base::Guard guard(m_lock);
	if (m_bindTele.find(strTelephone) == m_bindTele.end())
	{
		m_bindTele.insert(strTelephone);
	}
}

bool RegistMgr::removeTelephoneInfo(const string strTelephone)
{
	try
	{
		set<string>::iterator iter = m_bindTele.find(strTelephone);
		if (iter != m_bindTele.end())
		{
			m_bindTele.erase(strTelephone);
		}
	}
	catch(...)
	{
		m_pSproxy->m_pRollLog->error("RegistMgr::removeTelephoneInfo exception(%s)", strTelephone.c_str());
	}

	return true;
}

int RegistMgr::modifyUserRegistInfo(const int &uid, const string &puid, const string &oldname, const string &newname)
{
	int res = ERROR_CODE::EC_Succeed;
	base::Guard guard(m_lock);
	set<string>::iterator it = m_usedName.find(newname);
	if (it == m_usedName.end())
	{
		m_usedName.erase(oldname);
		m_usedName.insert(newname);
	}
	else res = ERROR_CODE::EC_Failed;

	return res;
}

bool RegistMgr::checkAccountId(const string &strAccId)
{
	bool bRes = false;
	base::Guard guard(m_lock);
	map<string,int>::iterator it = m_mapAccId2Uid.find(strAccId);
	if (it != m_mapAccId2Uid.end()) bRes = true;

	return bRes;
}

bool RegistMgr::checkUid(const int &uid)
{
	bool bRes = false;
	base::Guard guard(m_lock);
	map<int, string>::iterator it = m_mapUid2AccId.find(uid);
	if (it != m_mapUid2AccId.end()) bRes = true;

	return bRes;
}


int RegistMgr::getUidByAccId(string &strAccId, string &strSdkId, bool &bIsMsdkUser)
{
	int nUid = -1;
	base::Guard guard(m_lock);
	map<string, int>::iterator it = m_mapAccId2Uid.find(strAccId);
	if (it == m_mapAccId2Uid.end())
	{
		if (strSdkId.empty())
    	{
    		strSdkId = m_pSproxy->m_pHtMgr->curlGetMsdk(strAccId);
    	}
		if (strSdkId != "")
		{
			it = m_mapAccId2Uid.find(strSdkId);
			bIsMsdkUser = true;
		}
		
	}
	if (it != m_mapAccId2Uid.end()) 
	{
		nUid = it->second;
	}

	return nUid;
}
string RegistMgr::getAccIdByUid(const int &uid)
{
	string strAccId = "";
	base::Guard guard(m_lock);
	map<int, string>::iterator it = m_mapUid2AccId.find(uid);
	if (it != m_mapUid2AccId.end())
	{
		strAccId = it->second;
	}

	return strAccId;
}

string RegistMgr::createToken(string strAccId, string strAccToken)
{
    string strSource = strAccId+"_"+strAccToken+"_"+base::i2s(time(NULL));
	string strLoginToken = base::s2md5(strSource);
	m_pSproxy->m_pRollLog->debug("RegistMgr::createToken  strAccId=%s; strAccToken=%s; strLoginToken=%s",strAccId.c_str(), 
	strAccToken.c_str(),strLoginToken.c_str());
    base::Guard guard(m_lock);
    m_mapAccId2AccToken[strAccId] = strAccToken;
    m_mapAccId2LoginToken[strAccId] = strLoginToken;
    return strLoginToken;
}

string RegistMgr::getLoginToken(string strAccId)
{
	string strLoginToken = "";
	m_pSproxy->m_pRollLog->debug("RegistMgr::getLoginToken  strAccId=%s",strAccId.c_str());
    base::Guard guard(m_lock);
    std::map<string, string>::iterator it = m_mapAccId2LoginToken.find(strAccId);
    if(it != m_mapAccId2LoginToken.end())
    {
        strLoginToken = it->second;
    }
    m_pSproxy->m_pRollLog->debug("RegistMgr::getLoginToken strAccId=%s, strLoginToken=%s",strAccId.c_str(), strLoginToken.c_str());
    return strLoginToken;
}

string RegistMgr::getAccToken(const string &strAccId, const string &strSdkId)
{
	string strToken = "";
	m_pSproxy->m_pRollLog->debug("RegistMgr::getAccToken  strAccId=%s, sdk_id=%s",strAccId.c_str(), strSdkId.c_str());
    base::Guard guard(m_lock);
    std::map<string, string>::iterator it = m_mapAccId2AccToken.find(strAccId);
    if (it == m_mapAccId2AccToken.end())
    {
    	if (strSdkId != "")
    	{
    		it = m_mapAccId2AccToken.find(strSdkId);
    	}
    	
    }
    if (it != m_mapAccId2AccToken.end())
    {
        strToken = it->second;
    }
    m_pSproxy->m_pRollLog->debug("RegistMgr::getAccToken accId=%s,sdkid:%s, token=%s",strAccId.c_str(), strSdkId.c_str(), strToken.c_str());
    return strToken;
}

bool RegistMgr::checkLoginToken(string &strAccId, string &strSdkId, const string &strLoginToken)
{
	//strSkdId为主
	bool bCheck = false;
	m_pSproxy->m_pRollLog->debug("RegistMgr::getAccToken  strAccId=%s, strLoginToken:%s",strAccId.c_str(), strLoginToken.c_str());
    base::Guard guard(m_lock);
    std::map<string, string>::iterator it = m_mapAccId2LoginToken.find(strAccId);
    if(it == m_mapAccId2LoginToken.end())
    {
    	if (strSdkId.empty())
    	{
    		strSdkId = m_pSproxy->m_pHtMgr->curlGetMsdk(strAccId);
    	}
    	if (strSdkId != "")
    	{
    		it = m_mapAccId2LoginToken.find(strSdkId); //兼容旧数据  
    	}
    	
    }
    if (it != m_mapAccId2LoginToken.end())
    {
    	if (strLoginToken == it->second)
    	{
    		bCheck = true;
    	}
    }
    
    m_pSproxy->m_pRollLog->debug("RegistMgr::getAccToken strAccId=%s, bCheck=%d",strAccId.c_str(), bCheck);
    return bCheck;
}

int RegistMgr::dispatchUid(const string &puid)
{
    int nUid = -1;
	base::Guard guard(m_lock);
	m_nMaxUID++;
	nUid = m_nMaxUID;

	m_mapUid2AccId[nUid] = puid;
	m_mapAccId2Uid[puid] = nUid;

	return nUid;
}

int RegistMgr::dispatchFaceLook(int &nFaceId, int &isSys, int &nSex)
{
    nFaceId = 0;
    isSys = 0;
    nSex = 0;
    int res = ERROR_CODE::EC_Failed;
    base::Guard guard(m_lock);
    if(m_nCurVecFaceIdIndex >= m_vecFaceId.size())
    {
        m_nCurVecFaceIdIndex = 0;
    }
    int nFaceIdx = m_nCurVecFaceIdIndex++;
    nFaceId = m_vecFaceId[nFaceIdx];
    /*QueryParam param;
	param.tableName = "tbl_facelook";
	param.keyFieldName = "id";
	param.key = nFaceId;

    DataVector vecRecord;
    m_pDbMgr->get(param, vecRecord);
    if(!vecRecord.empty())
    {
		msgpack::unpacked unpack;	
	  	msgpack::unpack(&unpack, vecRecord[0].data(), vecRecord[0].size());  
	  	msgpack::object  obj1 = unpack.get();
        stringstream ss;
        ss << obj1;
        m_pSproxy->m_pRollLog->debug("RegistMgr::dispatchFaceLook(ss:%s)", ss.str().c_str());

        Json::Reader reader;
        Json::Value value;

        if (reader.parse(ss.str(), value))
        {
            isSys = value["is_system"].asInt();
            nSex = value["sex"].asInt();
            res = ERROR_CODE::EC_Succeed;
        }
    }*/
    //isSys = m_pSproxy->m_pDc->m_pFacelookConf->get_is_system(nFaceId);
    //nSex = m_pSproxy->m_pDc->m_pFacelookConf->get_sex(nFaceId);

	CsvConfig::Facelook facelookConf;
	if(CsvConfigMgr::getInstance().findFacelookByKey(nFaceId, facelookConf))
	{
		isSys = facelookConf.get_is_system();
		nSex = facelookConf.get_sex();
	}
    res = ERROR_CODE::EC_Succeed;

	return res;
}

int RegistMgr::checkUserLogin(proto10login::C2SAccountLoginBody & c2sAccLogin,ProtocolMgr& packHeader, int &nUid)
{
    //客户端发送10001连接游戏服的时候，调用这个函数校验用户信息
	m_pSproxy->m_pRollLog->debug("RegistMgr::checkUserLogin begining(accid:%s, sdkid:%s, token:%s)", \
		c2sAccLogin.account_id.c_str(), c2sAccLogin.sdk_id.c_str(), c2sAccLogin.token.c_str());
    nUid = -1;
	try 
	{	
		//SDK验证
		int res = checkUserSdkLogin(c2sAccLogin);
		if(res != ERROR_CODE::EC_Succeed)
		{
			return res;
		}

       	bool bIsMsdkUser = false;
		nUid = getUidByAccId(c2sAccLogin.account_id, c2sAccLogin.sdk_id, bIsMsdkUser);
		
		//正式服白名单是否开启
		res = checkWhiteList(nUid);
		if(res != ERROR_CODE::EC_Succeed)
		{
			return res;
		}
		
		m_pSproxy->m_pRollLog->debug("RegistMgr::checkUserLogin accid:%s, sdkid:%s", c2sAccLogin.account_id.c_str(), c2sAccLogin.sdk_id.c_str());
		bool bIsNewUser = false;
		bool bIsNewUserLogin = false;
		string strSdkId = c2sAccLogin.account_id;
		string strNick = "";
		string strFaceLook = "";
		string strFeel = "";
		string strPhone = "";
        string strUnionId = c2sAccLogin.union_id;
        string strOpenId = c2sAccLogin.open_id;
        string strSDKPhone = c2sAccLogin.phone;
		int nUserStatus = 0; //账号状态 0正常, 1封号
		int nSex = 0;
		int nStatusEndTime;
		string strExpressPhone;
		string strExpressName;
		string strExpressAddress;

		int registerTime = 0;
		int nOnlineTime = 0;
		int lastLogoutTime = 0;
		int lastLoginTime  = 0;
		int nLoginCnt = 0;
		
		string strIp = common_utils::ipv42s(packHeader.m_header.ext2);
		string strDid = c2sAccLogin.did;
		string strRegIp = strIp;
		string strRegDid = c2sAccLogin.did;
		string strRegDevice = c2sAccLogin.device;
		string strRegChannel = c2sAccLogin.channel; //注册渠道
		m_pSproxy->m_pRollLog->debug("RegistMgr::CheckUserLogin(strIp:%s, strDid:%s)", strIp.c_str(), strDid.c_str());

		if ((bIsMsdkUser) && (c2sAccLogin.sdk_id != ""))
		{
			strSdkId = c2sAccLogin.sdk_id;
		}
		
        //查看是否新用户
		if(nUid <= 0)
        {
        	//ip和设备的校验
        	CsvConfig::Common commonConf;
			if(CsvConfigMgr::getInstance().findCommonByKey(COMMONID_IPDEVICELIMIT, commonConf))
			{
				string strVal = commonConf.get_value();
				Json::Reader reader;
		        Json::Value value;

		        if (reader.parse(strVal, value))
		        {
		        	if(value["ip"].asInt() == 1)
	        		{
	        			int nIpLimitCnt = value["ipcnt"].asInt();
						if(m_mapIp2UidSet.find(strIp) != m_mapIp2UidSet.end())
						{
							int nIpSize = m_mapIp2UidSet[strIp].size();
							if(nIpSize >= nIpLimitCnt)
							{
								m_pSproxy->m_pRollLog->error("RegistMgr::CheckUserLogin nIpLimitCnt:%d, nIpSize:%d, EC_IPDEVICE_REG_LIMIT error!",nIpLimitCnt, nIpSize);	
								return ERROR_CODE::EC_IPDEVICE_REG_LIMIT;
							}
						}
	        		}
					if(value["device"].asInt() == 1)
					{
						int nDeviceLimitCnt = value["devicecnt"].asInt();
						if(m_mapDid2UidSet.find(strDid) != m_mapDid2UidSet.end())
						{
							int nDidSize = m_mapDid2UidSet[strDid].size();
							if(nDidSize >= nDeviceLimitCnt)
							{
								m_pSproxy->m_pRollLog->error("RegistMgr::CheckUserLogin nDeviceLimitCnt:%d, nDidSize:%d, EC_IPDEVICE_REG_LIMIT error!",nDeviceLimitCnt, nDidSize);	
								return ERROR_CODE::EC_IPDEVICE_REG_LIMIT;
							}
						}
						
					}
		        }
			}
        	bIsNewUser = true;
			bIsNewUserLogin = true;
		    nUid = dispatchUid(strSdkId); //分配uid
			m_pSproxy->m_pRollLog->debug("RegistMgr::checkUserLogin checkAccountId failed， new user (puid:%s, token:%s, newUid:%d)!", \
				c2sAccLogin.sdk_id.c_str(), c2sAccLogin.token.c_str(),nUid);

			CsvConfig::Common commonConfNick;
            if(c2sAccLogin.nick.empty())
        	{
				if(CsvConfigMgr::getInstance().findCommonByKey(COMMONID_DEFAULT_NICK, commonConfNick))
				{
					string strNickFormat = commonConfNick.get_value();
					vector<string> vecNick;
					vecNick.push_back(base::i2s(nUid));
					
					common_utils::formatMsg(strNickFormat, strNick, vecNick);
				}
				else
				{
					strNick = base::i2s(nUid);
				}
        	}
			else
			{
				strNick = c2sAccLogin.nick;
			}

			
			if(c2sAccLogin.facelook.empty())
			{
				int nFaceId = 0;
	            int isSys = 0;
	            dispatchFaceLook(nFaceId, isSys, nSex);
				strFaceLook = base::i2s(nFaceId);
			}
			else
			{
				strFaceLook = c2sAccLogin.facelook;
			}

			
			if(CsvConfigMgr::getInstance().findCommonByKey(COMMONID_DEFAULT_FEEL, commonConf))
			{
				strFeel = commonConf.get_value();
			}
			else
			{
				strFeel = "";
			}        	

			//ip设备id注册信息
			if(m_mapIp2UidSet.find(strIp) != m_mapIp2UidSet.end())
			{
				m_mapIp2UidSet[strIp].insert(nUid);
			}
			else
			{
				set<int> uidSet;
				uidSet.insert(nUid);
				m_mapIp2UidSet[strIp] = uidSet;
			}
			if(m_mapDid2UidSet.find(strDid) != m_mapDid2UidSet.end())
			{
				m_mapDid2UidSet[strDid].insert(nUid);
			}
			else
			{
				set<int> uidSet;
				uidSet.insert(nUid);
				m_mapDid2UidSet[strDid] = uidSet;
			}
		}
		else
		{
			QueryParam param;
			param.tableName = TBL_PLAYER + base::i2s(nUid%10);
			param.keyFieldName = "uid";
			param.key = nUid;

			DataVector vecRecord;
			m_pDbMgr->get(param, vecRecord);
			if(!vecRecord.empty())
			{
				msgpack::unpacked unpack;	
			  	msgpack::unpack(&unpack, vecRecord[0].data(), vecRecord[0].size());  
			  	msgpack::object  obj1 = unpack.get();
		        stringstream ss;
		        ss << obj1;
		        m_pSproxy->m_pRollLog->debug("RegistMgr::CheckUserLogin(ss:%s)", ss.str().c_str());

		        Json::Reader reader;
		        Json::Value value;

		        if (reader.parse(ss.str(), value))
		        {
		        	strFaceLook = value["facelook"].asString();
					strFeel = value["feel"].asString();  
					strNick = value["nick"].asString();
					nSex = value["gender"].asInt();
					strPhone = value["phone"].asString();
					if(value.isMember("sdk_phone"))
					{
						strSDKPhone = value["sdk_phone"].asString();
					}
					if(value.isMember("status"))
					{
						nUserStatus = value["status"].asInt();
					}
					strRegChannel = value["channel_id"].asString();
					if(value.isMember("express_phone"))
					{
						strExpressPhone = value["express_phone"].asString();
					}
					if(value.isMember("express_address"))
					{
						strExpressAddress = value["express_address"].asString();
					}
					if(value.isMember("express_name"))
					{
						strExpressName = value["express_name"].asString();
					}
		        }
			}
			else
			{ 
				//来的这里就是有错误了
				m_pSproxy->m_pRollLog->error("RegistMgr::CheckUserLogin error !! (nUid:%d, vecRecord:%d)",\
						nUid, vecRecord.size());
				//bIsNewUser = true;
			}

			//判断封号
			/*if(nUserStatus != 0)
			{
				m_pSproxy->m_pRollLog->error("RegistMgr::CheckUserLogin status err !! (nUid:%d, nUserStatus:%d)",\
						nUid, nUserStatus);
				return ERROR_CODE::EC_ACC_FROZEN;
			}*/

			//登录信息
			QueryParam paramLogin;
		    paramLogin.tableName = TBL_PLAYER_LOGININFO + base::i2s(nUid%10);
		    paramLogin.keyFieldName = "uid";
		    paramLogin.key = nUid;

		    DataVector vecRecordLogin;
		    m_pDbMgr->get(paramLogin, vecRecordLogin);
		    if(!vecRecordLogin.empty())
		    {
		        msgpack::unpacked unpack;   
		        msgpack::unpack(&unpack, vecRecordLogin[0].data(), vecRecordLogin[0].size());  
		        msgpack::object  obj = unpack.get();
		        stringstream ss;
		        ss << obj;
		        m_pSproxy->m_pRollLog->debug("RegistMgr::CheckUserLogin(ss:%s)", ss.str().c_str());

		        Json::Reader reader;
		        Json::Value value;

		        if (reader.parse(ss.str(), value))
		        {
		            registerTime = value["reg_time"].asInt();
		            nOnlineTime  = value["online_time"].asInt();
		            lastLogoutTime   = value["last_logout_time"].asInt();
		            lastLoginTime     = value["last_login_time"].asInt();
		            //strIp       = value["last_login_ip"].asString();     
					nLoginCnt = value["login_cnt"].asInt();
					if(value.isMember("reg_ip"))
					{
						strRegIp = value["reg_ip"].asString();    
					}
					if(value.isMember("reg_did"))
					{
						strRegDid = value["reg_did"].asString();    
					}
					if(value.isMember("reg_device"))
					{
						strRegDevice = value["reg_device"].asString();    
					}
		        }
		    }
			else
			{
				bIsNewUserLogin = true;
			}
		}
		
		//如果以前改过昵称头像，就不更新为微信昵称头像了
		updateNickFaceLook(bIsNewUser, nUid, c2sAccLogin, strNick, strFaceLook);

		//获取sdk的玩家电话号码记录，更新到游戏
		getUserMobile(c2sAccLogin, strPhone, strSDKPhone);

		//更新用户基础信息表
		QueryParam param;
    	param.tableName = TBL_PLAYER + base::i2s(nUid%10);
    	param.keyFieldName = "uid";
    	param.key = nUid;
        
        msgpack::sbuffer sbuf;
    	msgpack::packer<msgpack::sbuffer> pker(&sbuf);
    	pker.pack_map(22);
    	pker.pack(std::string("uid"));
    	pker.pack(nUid);
    	pker.pack(std::string("nick"));
		pker.pack(strNick);
    	pker.pack(std::string("account_id"));
    	pker.pack(strSdkId);
    	pker.pack(std::string("account_name"));
    	pker.pack(c2sAccLogin.accname);
        pker.pack(std::string("account_type"));
    	pker.pack(c2sAccLogin.agent);
        pker.pack(std::string("channel_id"));
    	pker.pack(strRegChannel); //注册渠道
        pker.pack(std::string("platform"));
    	pker.pack(base::i2s(c2sAccLogin.agent));
        pker.pack(std::string("phone"));
    	pker.pack(strPhone);
        pker.pack(std::string("user_type"));
    	pker.pack((int)USERTYPE_NORMAL);
        pker.pack(std::string("facelook"));
    	pker.pack(strFaceLook);
        pker.pack(std::string("gender"));
    	pker.pack(nSex); 
        pker.pack(std::string("feel"));
    	pker.pack(strFeel);
        pker.pack(std::string("token_sign"));
    	pker.pack(c2sAccLogin.token);
        pker.pack(std::string("token_time"));
    	pker.pack(time(NULL));
        pker.pack(std::string("union_id"));
        pker.pack(strUnionId);
        pker.pack(std::string("open_id"));
        pker.pack(strOpenId);
        pker.pack(std::string("sdk_phone"));
        pker.pack(strSDKPhone);
		pker.pack(std::string("status"));
    	pker.pack(nUserStatus);
		pker.pack(std::string("status_end_time"));
    	pker.pack(nStatusEndTime);

		pker.pack(std::string("express_phone"));
		pker.pack(strExpressPhone);
		pker.pack(std::string("express_address"));
		pker.pack(strExpressAddress);
		pker.pack(std::string("express_name"));
		pker.pack(strExpressName);
		pker.pack(std::string("exchange_phone"));
		pker.pack(strExpressName);
		//！！如果player表有加新字段，这里加了，必须在platform的playerinfo类也加上!!

    	m_pDbMgr->set(param, string(sbuf.data(), sbuf.size()),bIsNewUser);
		

		//把登录信息写入数据库
		QueryParam paramLogin;
		paramLogin.tableName = TBL_PLAYER_LOGININFO + base::i2s(nUid%10);
		paramLogin.keyFieldName = "uid";
		paramLogin.key = nUid;

		msgpack::sbuffer sbufLogin;
		msgpack::packer<msgpack::sbuffer> pkerLogin(&sbufLogin);
		pkerLogin.pack_map(14);
		pkerLogin.pack(std::string("uid"));
		pkerLogin.pack(nUid);
		pkerLogin.pack(std::string("reg_time"));
		pkerLogin.pack(registerTime == 0 ? (int)time(NULL) : registerTime);
		pkerLogin.pack(std::string("online_time"));
		pkerLogin.pack(nOnlineTime);
		pkerLogin.pack(std::string("last_logout_time"));
		pkerLogin.pack(lastLogoutTime);
	    pkerLogin.pack(std::string("last_login_time"));
		pkerLogin.pack(lastLoginTime);
	    pkerLogin.pack(std::string("last_login_ip"));
		pkerLogin.pack(strIp);
	    pkerLogin.pack(std::string("device"));
		pkerLogin.pack(c2sAccLogin.device);
	    pkerLogin.pack(std::string("did"));
		pkerLogin.pack(c2sAccLogin.did);
	    pkerLogin.pack(std::string("client_version"));
		pkerLogin.pack(c2sAccLogin.version);
		pkerLogin.pack(std::string("login_cnt"));
		pkerLogin.pack(nLoginCnt);

		pkerLogin.pack(std::string("reg_ip"));
		pkerLogin.pack(strRegIp);

		pkerLogin.pack(std::string("reg_did"));
		pkerLogin.pack(strRegDid);

		pkerLogin.pack(std::string("reg_device"));
		pkerLogin.pack(strRegDevice);

		//player表只存注册渠道, login表存登录渠道
		pkerLogin.pack(std::string("channel_id"));
		pkerLogin.pack(c2sAccLogin.channel);

		//！！如果player_login表有加新字段，这里加了，必须在platform的playerlogin类也加上!!
		
		m_pDbMgr->set(paramLogin, string(sbufLogin.data(), sbufLogin.size()),bIsNewUserLogin);

		//大数据日志
		if(bIsNewUser)
		{
			m_pSproxy->m_pBigDataLog->recordUserRegLog(c2sAccLogin.device, strSdkId,  nUid, c2sAccLogin.channel, c2sAccLogin.version);
			m_pSproxy->m_pBigDataLog->recordRoleRegLog(c2sAccLogin.device, strSdkId,  nUid, c2sAccLogin.channel, c2sAccLogin.version, strNick);
			m_pSproxy->m_pOperateLogMgr->recordUserRegLog(c2sAccLogin.device, strSdkId,  nUid, c2sAccLogin.channel, c2sAccLogin.version, "", c2sAccLogin.did, c2sAccLogin.did, strRegIp);
		}
		
		//为了大数据找玩家vip 等级
		int nVip = 0;
		int nLv = 0;
		if(!bIsNewUser)
		{
			QueryParam paramCTB;
		    paramCTB.tableName = TBL_PLAYER_CONTRIBUTION + base::i2s(nUid%10);
		    paramCTB.keyFieldName = "uid";
		    paramCTB.key = nUid;

		    DataVector vecRecordCTB;
		    m_pDbMgr->get(paramCTB, vecRecordCTB);
		    if(!vecRecordCTB.empty())
		    {
		        msgpack::unpacked unpack;   
		        msgpack::unpack(&unpack, vecRecordCTB[0].data(), vecRecordCTB[0].size());  
		        msgpack::object  obj = unpack.get();
		        stringstream ss;
		        ss << obj;
		        //m_pSproxy->m_pRollLog->debug("RegistMgr::CheckUserLogin(ss:%s)", ss.str().c_str());

		        Json::Reader reader;
		        Json::Value value;

		        if (reader.parse(ss.str(), value))
		        {
		        	nVip = value["vip"].asInt();
					nLv = value["level"].asInt();
		        }
		    }
		}
		
		m_pSproxy->m_pBigDataLog->recordUserActLog(1, 1, c2sAccLogin.device, lastLoginTime, nUid, c2sAccLogin.channel, c2sAccLogin.version, bIsNewUser, nVip, nLv);
		m_pSproxy->m_pBigDataLog->recordRoleActLog(1, 1, c2sAccLogin.device, lastLoginTime, nUid, c2sAccLogin.channel, c2sAccLogin.version, nVip, nLv);
		m_pSproxy->m_pOperateLogMgr->recordUserActLog(strSdkId, c2sAccLogin.device, c2sAccLogin.did, strRegIp,1,0,	nUid, c2sAccLogin.channel, c2sAccLogin.version);

    	   	m_pSproxy->m_pRollLog->debug("RegistMgr::checkUserLogin successfully(uid:%d)", nUid);
	}
    catch (std::exception& e) 
	{
		m_pSproxy->m_pRollLog->error("RegistMgr::CheckUserLogin account_id=%s : error=%s", c2sAccLogin.account_id, e.what());
	}
	catch (...)
	{
		m_pSproxy->m_pRollLog->error("RegistMgr::CheckUserLogin unknow exception");	
	}
    return ERROR_CODE::EC_Succeed;
}


int RegistMgr::checkWxLogin(proto10login::C2SWxLoginBody & c2s, proto10login::S2CWxLoginBody & s2c)
{
    //微信登录校验

	try 
	{	
	    //检测loginToken是否正确
        string strWxRet = m_pSproxy->m_pHtMgr->curlCheckWxLogin(c2s.code);
		if(strWxRet != "")
		{
			Json::Reader reader;
	        Json::Value root;
	        //从json字符串中读取数据
	        if (reader.parse(strWxRet,root))
	        {
	        	if(root.isMember("openid"))
        		{
        			s2c.code = ERROR_CODE::EC_Succeed;
					s2c.openid = root["openid"].asString();
					s2c.timestamp = time(NULL);
					s2c.token = createWxToken(s2c.openid, strWxRet);
        		}
	            else if(root.isMember("errcode"))
            	{
            		s2c.code = root["errcode"].asInt();
            	}
				else
				{
					s2c.code = ERROR_CODE::EC_Failed;
				}
	        }
		}
	}
	catch (std::exception& e) 
	{
		m_pSproxy->m_pRollLog->error("RegistMgr::CheckUserLogin : error=%s",  e.what());
	}
	catch (...)
	{
		m_pSproxy->m_pRollLog->error("RegistMgr::CheckUserLogin unknow exception");	
	}
    return ERROR_CODE::EC_Succeed;
}


string RegistMgr::createWxToken(string strOpenId, string strWxLoginJson)
{
    string strSource = strOpenId+"_"+strWxLoginJson+"_"+base::i2s(time(NULL));
	string strLoginToken = base::s2md5(strSource);
	m_pSproxy->m_pRollLog->debug("RegistMgr::createWxToken  strOpenId=%s; strWxLoginJson=%s; strLoginToken=%s",strOpenId.c_str(), 
	strWxLoginJson.c_str(),strLoginToken.c_str());
    base::Guard guard(m_lock);
    m_mapWxToken2LoginJson[strLoginToken] = strWxLoginJson;
    return strLoginToken;
}

void RegistMgr::getUserMobile(proto10login::C2SAccountLoginBody & c2sAccLogin, string &strPhone, string &strSdkPhone)
{
	//获取sdk的玩家电话号码记录，更新到游戏
	string strUserSdkId = c2sAccLogin.account_id;
	if(c2sAccLogin.device != "ios")
	{
		strUserSdkId = c2sAccLogin.sdk_id;
	}
	if(strUserSdkId != "")
	{
		//20181027 modify 全以sdk为准
		string strSdkRet = m_pSproxy->m_pHtMgr->curlGetUserPhone(strUserSdkId);
		if(strSdkRet != "-1")
		{
			strSdkPhone = strSdkRet;
			strPhone = strSdkRet;
		}
		/*strSdkPhone = m_pSproxy->m_pHtMgr->curlGetUserPhone(strUserSdkId);
		strPhone = strSdkPhone; 
		if(strSdkPhone != "")
		{
			strPhone = strSdkPhone;
		}
		else if(strPhone != "") //绑定到sdk
		{
			int nRes = m_pSproxy->m_pHtMgr->curlSaveBindPhone(strUserSdkId, strPhone);
			if(nRes == 0)
			{
				strSdkPhone = strPhone;
			}
		}*/
	}
}

void RegistMgr::updateNickFaceLook(bool bIsNewUser, int nUid,proto10login::C2SAccountLoginBody & c2sAccLogin, string &strNick, string &strFaceLook)
{
		//如果以前改过昵称头像，就不更新为微信昵称头像了
		int nChgNickCnt = 0;
		int nLastChangeFacelookTime = 0;
		if(!bIsNewUser)
		{
			QueryParam paramOper;
			paramOper.tableName = TBL_PLAYER_OPER_USERINFO + base::i2s(nUid%10);
			paramOper.keyFieldName = "uid";
			paramOper.key = nUid;

			DataVector vecRecordOper;
			m_pDbMgr->get(paramOper, vecRecordOper);
			if(!vecRecordOper.empty())
			{
				msgpack::unpacked unpack;	
			  	msgpack::unpack(&unpack, vecRecordOper[0].data(), vecRecordOper[0].size());  
			  	msgpack::object  obj1 = unpack.get();
		        stringstream ss;
		        ss << obj1;
		        m_pSproxy->m_pRollLog->debug("RegistMgr::CheckUserLogin(ss:%s)", ss.str().c_str());

		        Json::Reader reader;
		        Json::Value value;

		        if (reader.parse(ss.str(), value))
		        {
		        	nChgNickCnt = value["change_nick_cnt"].asInt();
					nLastChangeFacelookTime = value["last_change_facelook_time"].asInt();
		        }
			}
		}
		
		if(nChgNickCnt <= 0 && !c2sAccLogin.nick.empty())
		{
			strNick = c2sAccLogin.nick;
			if(strNick.size() > 45) //数据库50个字符，这里要截取
			{
				strNick = common_utils::Utf8SubStr(strNick, 45);
			}
		}
		if(nLastChangeFacelookTime <= 0 && !c2sAccLogin.facelook.empty())
		{
			strFaceLook = c2sAccLogin.facelook;
		}
}

int RegistMgr::checkUserSdkLogin(proto10login::C2SAccountLoginBody & c2sAccLogin)
{
	
	if(!checkLoginToken(c2sAccLogin.account_id, c2sAccLogin.sdk_id, c2sAccLogin.token))
	{
		//测试开关，专用的测试串，win32
		if (m_pSproxy->m_nTestOpen != 0 && c2sAccLogin.token == "46D6C2CF850EABEF76C371D792E6968D")
		{
			m_pSproxy->m_pRollLog->debug("RegistMgr::CheckUserLogin (TestOpen:%d, c2sAccLogin.token:%s) only win32 test!",\
					m_pSproxy->m_nTestOpen, c2sAccLogin.token.c_str());
		}
		else 
		{
			m_pSproxy->m_pRollLog->error("RegistMgr::CheckUserLogin m_nTestOpen != 0 check failed(puid:%s, c2sAccLogin.token:%s)!", \
				c2sAccLogin.account_id.c_str(), c2sAccLogin.token.c_str());
			m_pSproxy->m_pBigDataLog->recordUserActLog(1, 0, c2sAccLogin.device, time(NULL), 0, c2sAccLogin.channel, c2sAccLogin.version, true, 0, 0);
			return ERROR_CODE::EC_GAME_CHKLOGIN_ERR;
		}
	}
	//去SDK平台验证
    if (m_pSproxy->m_nTestOpen == 0)
	{
		//SDK验证
		bool bIsAndroid = false;
	    string strAccToken = getAccToken(c2sAccLogin.account_id, c2sAccLogin.sdk_id);
	    if(strAccToken.empty())
	    {
	    	m_pSproxy->m_pRollLog->error("RegistMgr::checkUserSdkLogin strAccToken check failed(puid:%s, strAccToken:%s)!", \
				c2sAccLogin.account_id.c_str(), strAccToken.c_str());
			m_pSproxy->m_pBigDataLog->recordUserActLog(1, 0, c2sAccLogin.device, time(NULL), 0, c2sAccLogin.channel, c2sAccLogin.version, true, 0, 0);
	        return ERROR_CODE::EC_GAME_CHKLOGIN_ERR;
	    }
		if (c2sAccLogin.device.find("and") != string::npos) //移动端安卓需要特殊验证
		{
			bIsAndroid = true;
		}

		if (!m_pSproxy->m_pHtMgr->curlCheckLogin(c2sAccLogin.account_id, strAccToken, bIsAndroid))
		{
			m_pSproxy->m_pRollLog->error("RegistMgr::checkUserSdkLogin third platform login check failed(puid:%s, strAccToken:%s)!", \
				c2sAccLogin.account_id.c_str(), strAccToken.c_str());
			//兼容腾讯云的sdk
			if (!m_pSproxy->m_pHtMgr->curlCheckLoginTX(c2sAccLogin.account_id, strAccToken, bIsAndroid))
			{
				m_pSproxy->m_pRollLog->error("RegistMgr::checkUserSdkLoginTX third platform login check failed(puid:%s, strAccToken:%s)!", \
					c2sAccLogin.account_id.c_str(), strAccToken.c_str());
				
				m_pSproxy->m_pBigDataLog->recordUserActLog(1, 0, c2sAccLogin.device, time(NULL), 0, c2sAccLogin.channel, c2sAccLogin.version, true, 0, 0);
				return ERROR_CODE::EC_SDK_CHKLOGIN_ERR;
			}
			
		}
	}
	return ERROR_CODE::EC_Succeed;
}

int RegistMgr::checkWhiteList(int nUid)
{
	//检测白名单开启等
	//int nCurTime = time(NULL);
	//if(nCurTime - m_nLastLoadSysInfoTime > 60)
	//{
		//m_pSproxy->m_pSysInfoMgr->load();
		//m_nLastLoadSysInfoTime = nCurTime;
	//}
	m_pSproxy->m_pSysInfoMgr->loadWhiteListOpen();
	int nOpen = m_pSproxy->m_pSysInfoMgr->getWhiteListOpen();
	if(nOpen != SYSINFO_WHITE_LIST_CLOSE) //白名单开启时
	{
		m_pSproxy->m_pSysInfoMgr->loadWhiteList();
		string strWl = m_pSproxy->m_pSysInfoMgr->getWhiteList();
		string strUid = base::i2s(nUid);
		vector<string> vecWhiteList;
		common_utils::split_string(strWl, vecWhiteList, "|");
		if(nUid > 0 && isValueInVec(strUid, vecWhiteList))
		{
			m_pSproxy->m_pRollLog->debug("RegistMgr::checkWhiteList whitelist open:%d, (nUid:%d, strWl:%s) in white list!", \
				nOpen, nUid, strWl.c_str());
			return ERROR_CODE::EC_Succeed;
		}
		m_pSproxy->m_pRollLog->error("RegistMgr::checkWhiteList whitelist open:%d, (nUid:%d, strWl:%s) not in white list!", \
				nOpen, nUid, strWl.c_str());
		return ERROR_CODE::EC_SVR_MAINTAINING;
	}
	
	return ERROR_CODE::EC_Succeed;
}

