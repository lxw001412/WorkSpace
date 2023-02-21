#include "pch.h"
#include "CJson.h"
JsonData::JsonData() {
	m_sn = nullptr;
	m_msgid = 1;
}
void JsonData::Init(const char* sn, bool Start)
{
	if(sn)m_sn = (char*)sn;
	if (Start)
	{//开始按键测试
		m_root["sn"] = *m_sn;
		m_root["qos"] = 0;
		m_root["ack"] = 0;
		m_root["msgid"] = m_msgid++;
		m_root["topic"] = 51;
		m_root["cmd"] = 30;
		m_root["desc"] = "开始摄像头测试";
	}
	else
	{//结束按键测试
		m_root.clear();
		m_root["sn"] = *m_sn;
		m_root["qos"] = 0;
		m_root["ack"] = 0;
		m_root["msgid"] = m_msgid++;
		m_root["topic"] = 51;
		m_root["cmd"] = 31;
		m_root["desc"] = "结束摄像头测试";
	}
}

std::string JsonData::Dump()
{
	std::unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter()); //从字符串中输出到Json文件
	return Json::writeString(writerBuilder, m_root);
}





