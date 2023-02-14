#pragma once
#include <string>
#include "json/json.h"

class MyJson
{
public:
	MyJson();
	void Init(const char* sn, bool Start = true);
	~MyJson(){}
	operator Json::Value() { return m_root;}
	std::string Dump();
private:
	char* m_sn;
	int m_msgid;
	Json::Value m_root;
	Json::StreamWriterBuilder writerBuilder;
};

