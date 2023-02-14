#ifndef _CLOG_H_
#define _CLOG_H_
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
class CLog
{
public:
	static std::shared_ptr<spdlog::logger> GetInstace();
private:
	CLog();
	~CLog();
protected:
	static std::shared_ptr<spdlog::logger> GetLogger();
	static void releaseInstance() {
		if (m_log != NULL) {
			CLog* tmp = m_log;
			m_log = NULL;
			delete tmp;
		}
		if (m_logger != NULL) {
			spdlog::drop_all();
		}
	}
private:
	static CLog* m_log;
	static std::shared_ptr<spdlog::logger> m_logger;
	class CHelper {
	public:
		CHelper() {
			CLog::GetInstace();
		}
		~CHelper() {
			CLog::releaseInstance();
		}
	};
	static CHelper m_helper;
};

//#ifdef DEBUG
//debug
#define LOG_D(...) SPDLOG_LOGGER_CALL(CLog::GetInstace(), spdlog::level::debug, __VA_ARGS__)
//info
#define LOG_I(...) SPDLOG_LOGGER_CALL(CLog::GetInstace(), spdlog::level::info, __VA_ARGS__)
//warn
#define LOG_W(...) SPDLOG_LOGGER_CALL(CLog::GetInstace(), spdlog::level::warn, __VA_ARGS__)
//error
#define LOG_E(...) SPDLOG_LOGGER_CALL(CLog::GetInstace(), spdlog::level::err, __VA_ARGS__)
//#endif // DEBUG

#endif //_COMMON_H_

