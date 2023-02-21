// MyThread.cpp: 实现文件
//
#include "pch.h"
#include "afxdialogex.h"
#include "CTestDialog.h"
#include "CTftp.h"
#include "CJson.h"
#include "videodecoder.h"
using namespace std;
void CheckTimeOut(void* arg);

IMPLEMENT_DYNAMIC(CTestDialog, CWorkDialog)

CTestDialog::CTestDialog(CWnd* pParent /*=nullptr*/)
{
	m_flag = true;
	m_status = false;
	m_result = false;
	m_Testing = false;
	m_sn = nullptr;
	m_tftp = nullptr;
	m_InetWork = nullptr;
}

void CTestDialog::Init(const YAML::Node conf, INetwork &network, const ACE_INET_Addr &devaddr, const ACE_INET_Addr &assistaddr, const char* devSN)
{
	if (!GetParent())return;
	m_sn = devSN;
	m_config = conf;
	CheckInfo();
	m_InetWork = &network;
	m_devaddr = devaddr;
	m_assistaddr = assistaddr;
	m_InetWork->addHandler(this);
	m_Thread = std::thread(CheckTimeOut, this);
	m_tftp = new TftpServer(CString2C(GetCurPath()));
	m_tftp->Start();
	SetBtnAttr(BtnAtr::Init);
	return;
}

void CTestDialog::CheckInfo()
{
	if ((!m_config["name"].IsDefined()) || (!m_config["enable"].IsDefined()) || (!m_config["library"].IsDefined()))
	{
		GetDlgItem(IDC_TOP)->SetWindowText(L"配置文件错误!");
		return;
	}
}

void CTestDialog::AutoTest()
{
	if (!GetParent())return;
	SetBtnAttr(BtnAtr::Start);
	m_status = false;
	GetDlgItem(IDC_TOP)->SetWindowText(L"正在测试中...");
	JsonData data;
	data.Init(m_sn);
	int len = m_InetWork->sendto(data, m_devaddr);
	if (len < 0)
	{
		GetDlgItem(IDC_TOP)->SetWindowText(L"请求发送失败,请检查网络!");
		SetBtnAttr(BtnAtr::Err);
		return;
	}
	else
	{
		m_Testing = true; //通知线程经检测超时
	}
}

int CTestDialog::handle(const Json::Value &data, const ACE_INET_Addr &from_addr)
{
	if (data.isNull())
	{
		LOG_E("handle data is null");
		return -1;
	}
	if (data["topic"].asInt() != 51 || (!m_Testing))return -2;
	m_status = true;   //通知数据已经收到
	StopTest();     
	std::string filename;
	filename.clear();
	filename = data["msg"]["filename"].asString();
	if (filename.empty())
	{
		LOG_E("收到的文件名无效!");
		m_Testing = false;
		SetBtnAttr(BtnAtr::Err);
		return -1;
	}
	//解码
	CString strpath = GetCurPath() + C2CString(filename.c_str());
	LOG_I(CString2C(strpath));
	CH264Decoder decode(CString2C(strpath));
	int re = decode.decode_h264();
	if (re < 0)
	{
		GetDlgItem(IDC_TOP)->SetWindowText(L"解码错误!");
		m_Testing = false;
		SetBtnAttr(BtnAtr::Err);
		return -2;
	}
	Render();
	m_Testing = false;
	SetBtnAttr(BtnAtr::End);
	return 0;
}

int CTestDialog::StopTest()
{
	JsonData data;
	data.Init(m_sn, false);
	int len = m_InetWork->sendto(data, m_devaddr);
	if (len < 0)
	{
		LOG_E("测试数据发送失败!");
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

void CTestDialog::SetBtnAttr(BtnAtr operate)
{
	if (!GetParent())return;
	if (operate == BtnAtr::Init) //初始化
	{
		GetDlgItem(IDC_PASSBTN)->EnableWindow(false);
		GetDlgItem(IDC_FAILBTN)->EnableWindow(false);
		GetDlgItem(IDC_REPEATBTN)->EnableWindow(true);
		GetDlgItem(IDC_REPEATBTN)->SetWindowText(_T("开始"));
	}
	else if (operate == BtnAtr::Start)//测试中
	{
		GetDlgItem(IDC_PASSBTN)->EnableWindow(false);
		GetDlgItem(IDC_FAILBTN)->EnableWindow(false);
		GetDlgItem(IDC_REPEATBTN)->EnableWindow(false);
		GetDlgItem(IDC_REPEATBTN)->SetWindowText(_T("重测"));
	}
	else if (operate == BtnAtr::End)//测试结束
	{
		GetDlgItem(IDC_PASSBTN)->EnableWindow(true);
		GetDlgItem(IDC_FAILBTN)->EnableWindow(true);
		GetDlgItem(IDC_REPEATBTN)->EnableWindow(true);
	}
	else if (operate == BtnAtr::Err)//测试错误
	{
		GetDlgItem(IDC_PASSBTN)->EnableWindow(false);
		GetDlgItem(IDC_FAILBTN)->EnableWindow(true);
		GetDlgItem(IDC_REPEATBTN)->EnableWindow(true);
	}
}

CTestDialog::~CTestDialog()
{
	Clear();
}

void CTestDialog::Clear()
{
	StopTest();
	m_flag = false;
	if (m_tftp)
	{
		delete m_tftp;
		m_tftp = nullptr;
	}
	if (m_Thread.joinable())
	{
		m_Thread.join();
	}
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
	//AutoTest();
	ManualTest();
}

void CTestDialog::ManualTest()
{
	//解码
	std::string filename;
	filename.clear();
	filename = "test.h264";
	CString strpath = GetCurPath() + C2CString(filename.c_str());
	LOG_I(CString2C(strpath));
	CH264Decoder decode(CString2C(strpath));
	int re = decode.decode_h264();
	if (re < 0)
	{
		GetDlgItem(IDC_TOP)->SetWindowText(L"解码错误!");
		m_Testing = false;
		SetBtnAttr(BtnAtr::Err);
		return;
	}
	Render();
	SetBtnAttr(BtnAtr::End);
	return;
}

void CTestDialog::Render()
{
	CImage image;
	const char* imgname = "test.jpg";
	CString imagfullpath = GetCurPath() + C2CString(imgname);
	image.Load(imagfullpath);
	//以下两个矩形主要作用是，获取对话框上面的Picture Control的width和height，
	//并设置到图片矩形rectPicture，根据图片矩形rectPicture对图片进行处理，
	//最后绘制图片到对话框上Picture Control上面
	CRect rectControl;                        //控件矩形对象
	CRect rectPicture;                        //图片矩形对象

	int x = image.GetWidth();
	int y = image.GetHeight();
	//Picture Control的ID为IDC_IMAGE
	CWnd  *pWnd = GetDlgItem(IDC_IMG);
	pWnd->GetClientRect(rectControl);


	CDC *pDc = GetDlgItem(IDC_IMG)->GetDC();
	SetStretchBltMode(pDc->m_hDC, STRETCH_HALFTONE);

	rectPicture = CRect(rectControl.TopLeft(), CSize((int)rectControl.Width(), (int)rectControl.Height()));

	((CStatic*)GetDlgItem(IDC_IMG))->SetBitmap(NULL);

	image.Draw(pDc->m_hDC, rectPicture);                //将图片绘制到Picture控件表示的矩形区域

	image.Destroy();
	pWnd->ReleaseDC(pDc);
}

void CheckTimeOut(void* arg)
{
	CTestDialog* thiz = (CTestDialog*)arg;
	if (!thiz)return;
	int timeout_;
	if (!thiz->m_config["timeout"].IsDefined())
	{
		timeout_ = 10;
	}
	else timeout_ = thiz->m_config["timeout"].as<int>();
	while (thiz->m_flag)
	{
		if (thiz->m_Testing) //开始检测测试
		{
			ULONGLONG cur = GetTickCount64();
			while (thiz->m_flag && thiz->m_Testing)
			{
				ULONGLONG now = GetTickCount64();
				if ((now - cur > timeout_ * 1000) && (thiz->m_flag) && (thiz->m_Testing))
				{
					if (!(thiz->m_status))//如果信息没有接收到
					{
						if ((thiz->m_flag) && (thiz->m_Testing))
						{
							thiz->GetDlgItem(IDC_TOP)->SetWindowText(L"超时不通过!");
							thiz->m_Testing = false;
							thiz->StopTest();
							thiz->SetBtnAttr(BtnAtr::Err);
							break;
						}
					}
					else
					{
						Sleep(1);
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
			Sleep(1);
		}
	}
}