#pragma once
#include "tftp_svr.h"
#include <string>

class MyTftp
{
public:
	MyTftp(std::string path) {
		WSADATA d;
		WSAStartup(0x0202, &d);
		m_path = path;
		m_tftp = nullptr;
	}
	void Start(){m_tftp = (tftpd*) tftpd_create("0.0.0.0", 69, m_path.c_str());}
	~MyTftp() 
	{ 
		WSACleanup(); 
		if (m_tftp)
		{
			delete m_tftp;
			m_tftp = nullptr;
		}
	}
private:
	std::string m_path;
	tftpd* m_tftp;
};