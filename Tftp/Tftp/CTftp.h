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
	}
	void Start(){ tftpd_create("0.0.0.0", 69, m_path.c_str());}
	~MyTftp() { WSACleanup(); }
private:
	std::string m_path;
};