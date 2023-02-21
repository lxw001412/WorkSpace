#pragma once
#include "resource.h"
#include "base.h"
#include <queue>
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef AFX_MANAGE_STATE
# undef AFX_MANAGE_STATE
#endif
#define AFX_MANAGE_STATE( p )

////////////////////MSG///////////////////  
#define WM_MODMSG_END       (WM_USER + 1883)		               // 测试结束消息
/////////////////////////////////////////
enum BtnAtr
{
	Init,
	Start,
	End,
	Err
};
class CWorkDialog;
class TftpServer;
class CTestDialog : public CWorkDialog, public IHandler
{
	DECLARE_DYNAMIC(CTestDialog)

public:
	CTestDialog(CWnd* pParent = nullptr);                           // 标准构造函数
	~CTestDialog();
	void SetWindowAttr();                                           // 设置测试窗口的大小及按件位置            
	bool GetReport(std::string &report);                            // 获取测试报告
	void Init(const YAML::Node conf, INetwork &network, const ACE_INET_Addr &devaddr, const ACE_INET_Addr &assistaddr, const char* devSN);
public:
	bool           m_status;                                        // 接收信息状态
	bool           m_flag;                                          // 检测线程退出
	bool           m_Testing;                                       // 测试状态
	YAML::Node     m_config;                                        // 配置文件
	void SetBtnAttr(BtnAtr operate);                                // 设置按钮属性
	int  StopTest();                                                // 停止测试 返回0停止成功 
public:
	afx_msg void OnBnClickedPassbtn();                              // 测试通过按钮
	afx_msg void OnBnClickedFailbtn();                              // 测试不通过按钮
	afx_msg void OnBnClickedRepeatbtn();                            // 测试重测按钮
protected: 
	void Clear();                                                   // 资源回收
	void AutoTest();                                                // 自动测试
	void CheckInfo();                                               // 检查配置信息
	void ManualTest();                                              // 手动测试
	void Render();
	virtual int handle(const Json::Value &data, const ACE_INET_Addr &from_addr);
	virtual int handle(const char* buf, const ACE_INET_Addr &from_addr, size_t size) { return 0; }
	virtual void DoDataExchange(CDataExchange* pDX);                // DDX/DDV 支持
	DECLARE_MESSAGE_MAP()
private:
	const char*   m_sn;                                             // 序列号
	bool          m_result;                                         // 测试结果
	TftpServer*   m_tftp;                                           // tftp服务
	std::thread   m_Thread;                                         // 超时检测线程
	INetwork*     m_InetWork;                                       // 网络模块
	ACE_INET_Addr m_devaddr;                                        // 测试设备
	ACE_INET_Addr m_assistaddr;                                     // 辅助设备
}; 
