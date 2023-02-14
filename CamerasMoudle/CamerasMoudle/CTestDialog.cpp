// MyThread.cpp: 实现文件
//
#include "pch.h"
#include "afxdialogex.h"
#include "CTestDialog.h"
#include "CTftp.h"
#include "CRender.h"
#include "videodecoder.h"
using namespace std;
C264Decoder* g_decode = nullptr;


IMPLEMENT_DYNAMIC(CTestDialog, CWorkDialog)

void TimeOut(void* arg)
{
	CTestDialog* thiz = (CTestDialog*)arg;
	if (!thiz)return;
	while (thiz->m_flag && thiz)
	{
		if (thiz->m_start && thiz) //开始检测测试
		{
			CString time;
			CTime tm = CTime::GetCurrentTime();
			CTimeSpan timeout(0, 0, 0, thiz->m_time);
			while (thiz->m_flag && thiz->m_start && thiz)
			{
				if (((CTime::GetCurrentTime() - tm) > timeout) && (thiz->m_flag != false))
				{
					if (thiz->m_status == false)//如果信息没有接收到
					{
						if (thiz->m_flag && thiz)
						{
							thiz->SetBtnAttr(3);
							thiz->GetDlgItem(IDC_TOP)->SetWindowText(L"测试不通过!");
							thiz->m_start = false;
							break;
						}
					}
				}
				else
				{
					Sleep(1);
				}
			}
		}
		else
		{
			Sleep(100);
		}
	}
}

int CTestDialog::handle(const Json::Value &data, const ACE_INET_Addr &from_addr)
{
	if (data.isNull())
	{
		LOG_E("handle data is null");
		return -1;
	}
	if (data["topic"].asInt() != 51 && (data["cmd"].asInt() != 32))return -2;
	Json::StreamWriterBuilder writerBuilder;
	std::unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter()); //从字符串中输出到Json文件
	std::string str = Json::writeString(writerBuilder, data);
	m_status = true;   //通知数据已经收到
	m_file.clear();
	m_file = data["msg"]["filename"].asString();
	if (m_file.empty())
	{
		LOG_E("收到的文件名无效!");
		return -1;
	}
	if (g_decode)
	{
		delete g_decode;
		g_decode = nullptr;
	}
	//解码
	CString strpath = GetCurPath() + C2CString(m_file.c_str());
	LOG_I(CString2C(strpath));
	g_decode = new C264Decoder(CString2C(strpath));
	int re = g_decode->Init();
	if (re < 0)
	{
		LOG_E("解码错误!");
		return -2;
	}
	SetBtnAttr(3);
	return 0;
}

CTestDialog::CTestDialog(CWnd* pParent /*=nullptr*/)
{
	m_auto = false;
	m_result = false;
	m_InetWork = nullptr;
	m_sn = nullptr;
	m_flag = true;
	m_start = false;
	m_status = false;
	m_time = 0;
	m_tftp = nullptr;
}

void CTestDialog::Init(const YAML::Node conf, INetwork &network, const ACE_INET_Addr &devaddr, const ACE_INET_Addr &assistaddr, const char* devSN)
{
	if (!GetParent() || (conf.IsNull()))
	{
		LOG_E("配置信息错误!");
		return;
	}
	m_sn = devSN;
	m_config = conf;
	CheckInfo();
	m_InetWork = &network;
	m_devaddr = devaddr;
	m_assistaddr = assistaddr;
	SetBtnAttr(1);
	m_func = m_auto == true ? bind(&CTestDialog::AutoTest, this) : bind(&CTestDialog::ManualTest, this);
	if (m_auto)
	{
		m_InetWork->addHandler(this);
		m_Thread = std::thread(TimeOut, this);
		m_tftp = new MyTftp(CString2C(GetCurPath()));
		m_tftp->Start();
	}
	return;
}

void CTestDialog::CheckInfo()
{
	if (!m_config["auto"].IsDefined())
	{
		LOG_E("配置文件错误!");
		GetDlgItem(IDC_TOP)->SetWindowText(L"配置文件错误");
		return;
	}
	m_auto = m_config["auto"].as<bool>();
	m_time = m_config["timeout"].as<int>();
}

void CTestDialog::AutoTest()
{
	if (!GetParent())return;
	m_status = false;
	SetBtnAttr(2);
	GetDlgItem(IDC_TOP)->SetWindowText(L"开始测试");
	m_data.Init(m_sn);
	int len = m_InetWork->sendto(m_data, m_devaddr);
	if (len < 0)
	{
		LOG_E("请求发送失败,请检查网络!");
		GetDlgItem(IDC_TOP)->SetWindowText(L"请求发送失败,请检查网络!");
		LOG_I(m_data.Dump().c_str());
		SetBtnAttr(3);
		return;
	}
	else
	{
		m_start = true; //通知线程经检测超时
	}
}

void CTestDialog::HandleResult()
{
	return;

}

int CTestDialog::StopTest()
{
	m_data.Init(m_sn, false);
	int len = m_InetWork->sendto(m_data, m_devaddr);
	if (len < 0)
	{
		LOG_E("测试数据发送失败!");
		LOG_I(m_data.Dump().c_str());
		return -1;
	}
	return 0;
}

void CTestDialog::SetWindowAttr()
{
	this->MoveWindow(0, 0, 563, 550);
	CRect rect;
	GetClientRect(rect);
	if (rect.IsRectNull())return;
	GetDlgItem(IDC_PASSBTN)->MoveWindow(rect.left + 15, rect.top + 520, 70, 25);
	GetDlgItem(IDC_FAILBTN)->MoveWindow(rect.left + 480, rect.top + 520, 70, 25);
	GetDlgItem(IDC_REPEATBTN)->MoveWindow(rect.left + 380, rect.top + 520, 70, 25);
}

bool CTestDialog::GetReport(std::string & report)
{

	report = YAML::Dump(m_config);
	return m_result;
}

void CTestDialog::SetBtnAttr(int status)         //自动 true 测试中 false 测试完成
{                                                 //手动 true 初始化 false 测试中/测试完成
	if (!GetParent())return;
	if (m_auto) //自动
	{
		if (status == 1) //初始化
		{
			GetDlgItem(IDC_PASSBTN)->EnableWindow(false);
			GetDlgItem(IDC_FAILBTN)->EnableWindow(false);
			GetDlgItem(IDC_REPEATBTN)->EnableWindow(true);
			GetDlgItem(IDC_REPEATBTN)->SetWindowText(_T("开始"));
		}
		else if (status == 2)//测试中
		{
			GetDlgItem(IDC_PASSBTN)->EnableWindow(false);
			GetDlgItem(IDC_FAILBTN)->EnableWindow(false);
			GetDlgItem(IDC_REPEATBTN)->EnableWindow(false);
			GetDlgItem(IDC_REPEATBTN)->SetWindowText(_T("重测"));
		}
		else if (status == 3)//测试结束
		{
			GetDlgItem(IDC_PASSBTN)->EnableWindow(false);
			GetDlgItem(IDC_FAILBTN)->EnableWindow(true);
			GetDlgItem(IDC_REPEATBTN)->EnableWindow(true);
		}
	}
}

CTestDialog::~CTestDialog()
{
	m_flag = false;
	m_start = false;

	if (g_decode)
	{
		delete g_decode;
		g_decode = nullptr;
	}
	if (m_tftp)
	{
		delete m_tftp;
		m_tftp = nullptr;
	}
	if (m_Thread.joinable())
	{
		m_Thread.join();
	}
	LOG_I("TEST EXIT");
	m_InetWork->rmHandler(this);
}

void CTestDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTestDialog, CWorkDialog)
	ON_BN_CLICKED(IDC_PASSBTN, &CTestDialog::OnBnClickedPassbtn)
	ON_BN_CLICKED(IDC_FAILBTN, &CTestDialog::OnBnClickedFailbtn)
	ON_BN_CLICKED(IDC_REPEATBTN, &CTestDialog::OnBnClickedRepeatbtn)
END_MESSAGE_MAP()

void CTestDialog::OnBnClickedPassbtn()
{
	if (!GetParent())return;
	m_result = true;
	GetParent()->PostMessage(WM_MODMSG_END, NULL, NULL);
}

void CTestDialog::OnBnClickedFailbtn()
{
	if (!GetParent())return;
	m_result = false;
	GetParent()->PostMessage(WM_MODMSG_END, NULL, NULL);
}

void CTestDialog::OnBnClickedRepeatbtn()
{
	if (!GetParent())return;
	m_func();
}

void CTestDialog::ManualTest()
{
	if (g_decode)
	{
		delete g_decode;
		g_decode = nullptr;
	}
	//解码
	m_file = "test.h264";
	CString strpath = GetCurPath() + C2CString(m_file.c_str());
	LOG_I(CString2C(strpath));
	g_decode = new C264Decoder(CString2C(strpath));
	int re = g_decode->Init();
	if (re < 0)
	{
		LOG_E("解码错误!");
		return ;
	}
	SetBtnAttr(3);
	return;
}
