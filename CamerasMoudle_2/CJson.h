#pragma once
#include <string>
#include "json/json.h"

class JsonData
{
public:
	JsonData();
	void Init(const char* sn, bool Start = true);
	~JsonData(){}
	operator Json::Value() { return m_root;}
	std::string Dump();
private:
	char* m_sn;
	int m_msgid;
	Json::Value m_root;
	Json::StreamWriterBuilder writerBuilder;
};

