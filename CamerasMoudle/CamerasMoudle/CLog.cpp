#include "CLog.h"
#include <cstdlib>
#include <Ctime>

CLog* CLog::m_log = nullptr;
std::shared_ptr<spdlog::logger> CLog::m_logger = nullptr;
CLog::CHelper CLog::m_helper;

std::shared_ptr<spdlog::logger> CLog::GetInstace()
{
	if (m_log)return m_logger;
	m_log = new CLog();
	return m_log->GetLogger();
}

std::shared_ptr<spdlog::logger> CLog::GetLogger()
{
	if(!m_logger)return std::shared_ptr<spdlog::logger>();
	return m_logger;
}

CLog::CLog()
{
	if (!m_logger)
	{
		//10M一个文件，最多存3个文件
		m_logger = spdlog::rotating_logger_st("CamerasMoudle", "CamerasMoudle.txt", 1048576 * 10, 3);
		m_logger->set_level(spdlog::level::level_enum::debug);
		//info日志,实时写入文件
		m_logger->flush_on(spdlog::level::level_enum::info);
	}
}

CLog::~CLog(){}
