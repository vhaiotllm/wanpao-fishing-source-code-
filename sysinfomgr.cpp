#include "sysinfomgr.h"
#include "loginserver.h"

SysInfoMgr::SysInfoMgr()
{

}

SysInfoMgr::~SysInfoMgr()
{

}

void SysInfoMgr::init(LoginServer *p_proxy)
{
	m_pSproxy = p_proxy;
	m_pDbMgr  = m_pSproxy->m_pDbMgr;

	m_bIsSysNewSvrStopTime = false;
	m_bIsSysNewWhiteList = false;
	m_bIsSysNewWhiteListOpen = false;
	load();
}

void SysInfoMgr::load()
{
	//loadSvrStopTime();

	loadWhiteListOpen();

	loadWhiteList();
	
}

void SysInfoMgr::loadSvrStopTime()
{
	QueryParam paraRank;
	paraRank.tableName = TBL_SYS_INFO; 
	paraRank.keyFieldName = "itemid";
	paraRank.key = SYSINFO_ITEMID_SVR_STOP_TIME;
	paraRank.userIdFiledName = "svrid";
	paraRank.userid = m_pSproxy->m_pUserSvrCache->getDefaultPlatformSvrId(); //使用默认的大厅，一个就够了无需每个大厅都记录
	
	DataVector vecRecordRank;
    m_pDbMgr->get(paraRank, vecRecordRank);
    if(!vecRecordRank.empty())
    {
		msgpack::unpacked unpack;	
	  	msgpack::unpack(&unpack, vecRecordRank[0].data(), vecRecordRank[0].size());  
	  	msgpack::object  obj1 = unpack.get();
        stringstream ss;
        ss << obj1;
        m_pSproxy->m_pRollLog->debug("SysInfoMgr::load(ss:%s)", ss.str().c_str());

        Json::Reader reader;
        Json::Value value;

        if (reader.parse(ss.str(), value))
        {
        	m_nSvrStopTime = base::s2i(value["itemvalue"].asString());
    	}
	}
	else
	{
		m_bIsSysNewSvrStopTime = true;
		updateSvrStopTime(0); 
		m_bIsSysNewSvrStopTime = false;
	}
}

int SysInfoMgr::updateSvrStopTime(int nTime)
{
	//base::Guard guard(m_lockRank) ;
	m_nSvrStopTime = nTime;
		
	//更新表数据
	QueryParam para;
	para.tableName = TBL_SYS_INFO; 
	para.keyFieldName = "itemid";
	para.key = SYSINFO_ITEMID_SVR_STOP_TIME;
	para.userIdFiledName = "svrid";
	para.userid = m_pSproxy->m_pUserSvrCache->getDefaultPlatformSvrId(); //使用默认的大厅，一个就够了无需每个大厅都记录
	
	msgpack::sbuffer sbuf;
	msgpack::packer<msgpack::sbuffer> pker(&sbuf);
	pker.pack_map(4);
	pker.pack(std::string("svrid"));
	pker.pack(m_pSproxy->m_pUserSvrCache->getDefaultPlatformSvrId()); //使用默认的大厅，一个就够了无需每个大厅都记录
	pker.pack(std::string("itemid"));
	pker.pack((int)SYSINFO_ITEMID_SVR_STOP_TIME);
	pker.pack(std::string("itemvalue"));
	pker.pack(base::i2s(m_nSvrStopTime));
	pker.pack(std::string("updated"));
	pker.pack((int)time(NULL));

	m_pDbMgr->set(para, string(sbuf.data(), sbuf.size()), m_bIsSysNewSvrStopTime);

	m_pSproxy->m_pRollLog->debug("SysInfoMgr::updateSvrStopTime m_nSvrStopTime:%d",m_nSvrStopTime);

	return m_nSvrStopTime;
}

int SysInfoMgr::getSvrStopTime()
{
	//base::Guard guard(m_lockRank) ;
	return m_nSvrStopTime;
}

void SysInfoMgr::loadWhiteListOpen()
{
	QueryParam paraRank;
	paraRank.tableName = TBL_SYS_INFO; 
	paraRank.keyFieldName = "itemid";
	paraRank.key = SYSINFO_ITEMID_WHITE_LIST_OPEN;
	paraRank.userIdFiledName = "svrid";
	paraRank.userid = m_pSproxy->m_pUserSvrCache->getDefaultPlatformSvrId(); //使用默认的大厅，一个就够了无需每个大厅都记录
	
	DataVector vecRecordRank;
    m_pDbMgr->get(paraRank, vecRecordRank);
    if(!vecRecordRank.empty())
    {
		msgpack::unpacked unpack;	
	  	msgpack::unpack(&unpack, vecRecordRank[0].data(), vecRecordRank[0].size());  
	  	msgpack::object  obj1 = unpack.get();
        stringstream ss;
        ss << obj1;
        m_pSproxy->m_pRollLog->debug("SysInfoMgr::load(ss:%s)", ss.str().c_str());

        Json::Reader reader;
        Json::Value value;

        if (reader.parse(ss.str(), value))
        {
        	m_nWhiteListOpen = base::s2i(value["itemvalue"].asString());
    	}
	}
	else
	{
		m_bIsSysNewWhiteListOpen = true;
		updateWhiteListOpen(0); 
		m_bIsSysNewWhiteListOpen = false;
	}
}

int SysInfoMgr::updateWhiteListOpen(int nOpen)
{
	//base::Guard guard(m_lockRank) ;
	m_nWhiteListOpen = nOpen;
		
	//更新表数据
	QueryParam para;
	para.tableName = TBL_SYS_INFO; 
	para.keyFieldName = "itemid";
	para.key = SYSINFO_ITEMID_WHITE_LIST_OPEN;
	para.userIdFiledName = "svrid";
	para.userid = m_pSproxy->m_pUserSvrCache->getDefaultPlatformSvrId(); //使用默认的大厅，一个就够了无需每个大厅都记录
	
	msgpack::sbuffer sbuf;
	msgpack::packer<msgpack::sbuffer> pker(&sbuf);
	pker.pack_map(4);
	pker.pack(std::string("svrid"));
	pker.pack(m_pSproxy->m_pUserSvrCache->getDefaultPlatformSvrId()); //使用默认的大厅，一个就够了无需每个大厅都记录
	pker.pack(std::string("itemid"));
	pker.pack((int)SYSINFO_ITEMID_WHITE_LIST_OPEN);
	pker.pack(std::string("itemvalue"));
	pker.pack(base::i2s(m_nWhiteListOpen));
	pker.pack(std::string("updated"));
	pker.pack((int)time(NULL));

	m_pDbMgr->set(para, string(sbuf.data(), sbuf.size()), m_bIsSysNewWhiteListOpen);

	m_pSproxy->m_pRollLog->debug("SysInfoMgr::updateWhiteListOpen m_nWhiteListOpen:%d",m_nWhiteListOpen);

	return m_nWhiteListOpen;
}

int SysInfoMgr::getWhiteListOpen()
{
	//base::Guard guard(m_lockRank) ;
	return m_nWhiteListOpen;
}

void SysInfoMgr::loadWhiteList()
{
	QueryParam paraRank;
	paraRank.tableName = TBL_SYS_INFO; 
	paraRank.keyFieldName = "itemid";
	paraRank.key = SYSINFO_ITEMID_WHITE_LIST;
	paraRank.userIdFiledName = "svrid";
	paraRank.userid = m_pSproxy->m_pUserSvrCache->getDefaultPlatformSvrId(); //使用默认的大厅，一个就够了无需每个大厅都记录
	
	DataVector vecRecordRank;
    m_pDbMgr->get(paraRank, vecRecordRank);
    if(!vecRecordRank.empty())
    {
		msgpack::unpacked unpack;	
	  	msgpack::unpack(&unpack, vecRecordRank[0].data(), vecRecordRank[0].size());  
	  	msgpack::object  obj1 = unpack.get();
        stringstream ss;
        ss << obj1;
        m_pSproxy->m_pRollLog->debug("SysInfoMgr::load(ss:%s)", ss.str().c_str());

        Json::Reader reader;
        Json::Value value;

        if (reader.parse(ss.str(), value))
        {
        	m_strWhiteList = value["itemvalue"].asString();
    	}
	}
	else
	{
		m_bIsSysNewWhiteList = true;
		updateWhiteList(""); 
		m_bIsSysNewWhiteList = false;
	}
}

string SysInfoMgr::updateWhiteList(string strWhiteList)
{
	//base::Guard guard(m_lockRank) ;
	m_strWhiteList = strWhiteList;
		
	//更新表数据
	QueryParam para;
	para.tableName = TBL_SYS_INFO; 
	para.keyFieldName = "itemid";
	para.key = SYSINFO_ITEMID_WHITE_LIST;
	para.userIdFiledName = "svrid";
	para.userid = m_pSproxy->m_pUserSvrCache->getDefaultPlatformSvrId(); //使用默认的大厅，一个就够了无需每个大厅都记录
	
	msgpack::sbuffer sbuf;
	msgpack::packer<msgpack::sbuffer> pker(&sbuf);
	pker.pack_map(4);
	pker.pack(std::string("svrid"));
	pker.pack(m_pSproxy->m_pUserSvrCache->getDefaultPlatformSvrId()); //使用默认的大厅，一个就够了无需每个大厅都记录
	pker.pack(std::string("itemid"));
	pker.pack((int)SYSINFO_ITEMID_WHITE_LIST);
	pker.pack(std::string("itemvalue"));
	pker.pack(m_strWhiteList);
	pker.pack(std::string("updated"));
	pker.pack((int)time(NULL));

	m_pDbMgr->set(para, string(sbuf.data(), sbuf.size()), m_bIsSysNewWhiteList);

	m_pSproxy->m_pRollLog->debug("SysInfoMgr::updateWhiteList m_strWhiteList:%s",m_strWhiteList.c_str());

	return m_strWhiteList;
}

string SysInfoMgr::getWhiteList()
{
	//base::Guard guard(m_lockRank) ;
	return m_strWhiteList;
}