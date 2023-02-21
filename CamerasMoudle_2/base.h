#pragma once
#include "framework.h"
#include "yaml-cpp/yaml.h"
#include "ace/Event_Handler.h"
#include "ace/INET_Addr.h"
#include "json/json.h"
#include "CLog.h"
#include "Unicode.h"

#define NAME "PressBtnMoudle"
#define VERSION "1.0"

class IHandler
{
public:
	virtual int handle(const Json::Value &data, const ACE_INET_Addr &from_addr) = 0;
	virtual int handle(const char* buf, const ACE_INET_Addr &from_addr, size_t size) = 0;
};

class INetwork
{
public:
	virtual int sendto(const Json::Value& data, const ACE_INET_Addr &to_addr) = 0;
	virtual int sendto(const char* buf, const ACE_INET_Addr &to_addr, size_t size) = 0;
	virtual void addHandler(IHandler *handler) = 0;
	virtual void rmHandler(IHandler *handler) = 0;
};

struct ModInfo
{
public:
	ModInfo()
	{
		Name = NAME;
		Version = VERSION;
	}
	ModInfo(const std::string name_, const std::string version_)
	{
		Name = name_;
		Version = version_;
	}
	std::string Name;				                                      // 测试项名称
	std::string Version;			                                      // 测试项当前版本
};

class CWorkDialog : public CDialogEx
{
public:
	CWorkDialog(CWnd* pParent = nullptr) {}
	virtual ~CWorkDialog() {};
	virtual void Init(const YAML::Node conf, INetwork &network, const ACE_INET_Addr &devaddr, const ACE_INET_Addr &assistaddr, const char* devSN) = 0;
	virtual bool GetReport(std::string &report) = 0;
};

class CConfigDialog : public CDialogEx
{
public:
	CConfigDialog() {}
	virtual void Init(const YAML::Node& config) = 0;
	virtual std::string GetConfig() = 0;
	virtual ~CConfigDialog() {}
};

