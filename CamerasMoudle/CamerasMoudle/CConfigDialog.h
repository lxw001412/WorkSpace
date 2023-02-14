#pragma once
#include "base.h"
#ifdef AFX_MANAGE_STATE
# undef AFX_MANAGE_STATE
#endif
#define AFX_MANAGE_STATE( p )
     
#define WM_ModMSG_SAVE (WM_USER + 1884)		           // 测试项配置保存消息
// CConfigDialog 对话框

class CConfig : public CConfigDialog
{
	DECLARE_DYNAMIC(CConfig)

public:
	CConfig(CWnd* pParent = nullptr);                   // 标准构造函数
	void Init(const YAML::Node& config);
	void SetWindowAttr();
	std::string GetConfig();
	~CConfig();
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CONFIGDIALOG };
#endif

protected:                     
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	DECLARE_MESSAGE_MAP()
private:
	YAML::Node m_config;
public:
	afx_msg void OnBnClickedSave();
};
