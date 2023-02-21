#pragma once
#include "framework.h"
class CThread
{
public:
	CThread() {
		m_bStatus = false;
		m_hThread = INVALID_HANDLE_VALUE;
	}
	CThread(const CThread&) = delete;
	CThread& operator = (const CThread&) = delete;
	~CThread() {
		Stop();
	}
public:
	//true 表示成功 false表示失败
	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&CThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	bool IsValid() {//返回true表示有效 返回false表示线程异常或者已经终止
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE))return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	bool Stop() {
		if (m_bStatus == false)return true;
		m_bStatus = false;
		bool ret = WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;
		return ret;
	}
private:
	virtual void ThreadWorker() = 0;
	static void ThreadEntry(void* arg) {
		CThread* thiz = (CThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}
private:
	bool m_bStatus;
	HANDLE m_hThread;
};