// CConfigDialog.cpp: 实现文件
//
#include "pch.h"
#include "resource.h"
#include "CConfigDialog.h"
#include "afxdialogex.h"
using namespace std;
// CConfigDialog 对话框

IMPLEMENT_DYNAMIC(CConfig, CConfigDialog)

CConfig::CConfig(CWnd* pParent /*=nullptr*/) {}

void CConfig::Init(const YAML::Node& config)
{
	if (config.IsNull())
	{
		LOG_E("配置文件为空!");
		return;
	}
	m_config = config;
	SetWindowAttr();
}

void CConfig::SetWindowAttr()
{
	if (!GetParent())return;
	CRect rect;
	GetParent()->GetClientRect(rect);              //获得窗体的大小
	if (rect.IsRectNull())return;
	GetDlgItem(IDC_P)->MoveWindow(rect.left + 50, rect.top + 50, 100, 20);
	GetDlgItem(IDC_PRONAME)->MoveWindow(rect.left + 300, rect.top + 50, 100, 20);
	GetDlgItem(IDC_T)->MoveWindow(rect.left + 50, rect.top + 100, 100, 20);
	GetDlgItem(IDC_TESTNAME)->MoveWindow(rect.left + 300, rect.top + 100, 100, 20);
	GetDlgItem(IDC_A)->MoveWindow(rect.left + 50, rect.top + 150, 100, 20);
	GetDlgItem(IDC_AUTOTEST)->MoveWindow(rect.left + 300, rect.top + 150, 100, 20);
	GetDlgItem(IDC_C)->MoveWindow(rect.left + 50, rect.top + 200, 100, 20);
	GetDlgItem(IDC_TESTCHECK)->MoveWindow(rect.left + 300, rect.top + 200, 100, 20);
	GetDlgItem(IDC_D)->MoveWindow(rect.left + 50, rect.top + 250, 100, 20);
	GetDlgItem(IDC_DLLNAME)->MoveWindow(rect.left + 300, rect.top + 250, 100, 20);
	GetDlgItem(IDC_V)->MoveWindow(rect.left + 50, rect.top + 300, 100, 20);
	GetDlgItem(IDC_VERSION)->MoveWindow(rect.left + 300, rect.top + 300, 100, 20);
	GetDlgItem(IDC_O)->MoveWindow(rect.left + 50, rect.top + 350, 100, 20);
	GetDlgItem(IDC_TIMEOUTEDIT)->MoveWindow(rect.left + 300, rect.top + 350, 100, 20);
	GetDlgItem(ID_SAVE)->MoveWindow(rect.left + 450, rect.top + 350, 70, 20);

	const auto node_ = m_config;
	if (!node_["name"].IsDefined() || !node_["auto"].IsDefined() || !node_["enable"].IsDefined() || !node_["library"].IsDefined() || !node_["timeout"].IsDefined())
	{
		LOG_E("配置文件error!");
		return;
	}
	std::string nodeStr;
	const char* file;
	CString name;
	for (const auto node : node_)
	{
		nodeStr = node.first.as<string>();
		if (nodeStr == "name")
		{
			file = node.second.as<string>().c_str();
			name = UTF8toUnicode(file);
			GetDlgItem(IDC_TESTNAME)->SetWindowText(name);              //测试项名称
		}
		else if (nodeStr == "auto")
		{
			bool status = node.second.as<bool>();
			nodeStr = status == true ? "自动测试" : "手动测试";
			file = nodeStr.c_str();
			name = C2CString(file);
			GetDlgItem(IDC_AUTOTEST)->SetWindowText(name.GetBuffer());
		}
		else if (nodeStr == "enable")
		{
			((CButton*)GetDlgItem(IDC_TESTCHECK))->SetCheck(node.second.as<bool>());
		}
		else if (nodeStr == "version")
		{
			file = node.second.as<string>().c_str();
			name = C2CString(file);
			GetDlgItem(IDC_VERSION)->SetWindowText(name.GetBuffer());             //版本号
		}
		else if (nodeStr == "library")
		{
			file = node.second.as<string>().c_str();
			name = C2CString(file);
			GetDlgItem(IDC_DLLNAME)->SetWindowText(name.GetBuffer());            //动态库名称
		}
		else if (nodeStr == "timeout")
		{
			file = node.second.as<string>().c_str();
			name = C2CString(file);
			GetDlgItem(IDC_TIMEOUTEDIT)->SetWindowText(name.GetBuffer());
		}
	}
}

std::string CConfig::GetConfig() {
	if (!GetParent() || m_config.IsNull() || !m_config["name"].IsDefined() || !m_config["version"].IsDefined())return std::string();
	return YAML::Dump(m_config);
}

CConfig::~CConfig()
{
}

void CConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfig, CConfigDialog)
	ON_BN_CLICKED(ID_SAVE, &CConfig::OnBnClickedSave)
END_MESSAGE_MAP()


// CConfigDialog 消息处理程序


void CConfig::OnBnClickedSave()
{
	if (!GetParent() || m_config.IsNull())
	{
		LOG_E("未加载配置文件!");
		return;
	}
	CString str;
	GetDlgItem(IDC_TIMEOUTEDIT)->GetWindowText(str);
	const char*  timeout = CString2C(str);
	m_config["timeout"] = atoi(timeout);

	int status = ((CButton*)GetDlgItem(IDC_TESTCHECK))->GetCheck();
	bool result = status == 1 ? true : false;
	m_config["enable"] = result;
	GetParent()->PostMessage(WM_ModMSG_SAVE, NULL, NULL);
}

