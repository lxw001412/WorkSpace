#include "pch.h"
#include "CExport.h"
#include "resource.h"

class CWorkDialog;

extern"C" DLL_API ModInfo * getModInfo(CConfigDialog* dlg)
{
	if (!dlg)return nullptr;
	ModInfo* mode = new ModInfo();
	if (!mode)return nullptr;
	return mode;
}

extern"C" DLL_API CWorkDialog * getWorkDialog(CWnd * pParent)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if(!pParent)return nullptr;
	CTestDialog* Workdlg = nullptr;
	Workdlg = new CTestDialog(pParent);
	if (!Workdlg)return nullptr;
	if (!Workdlg->Create(IDD_TESTDIALOG, pParent))
	{
		LOG_E("窗口创建失败!");
		return nullptr;
	}
	Workdlg->SetWindowAttr();
	Workdlg->SetParent(pParent);
	return Workdlg;
}

extern"C" DLL_API CConfigDialog * getConfigDialog(CWnd * pParent)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (!pParent)return nullptr;
	CConfigDialog* Configdlg = nullptr;
	Configdlg = new CConfig(pParent);
	if (!Configdlg)return nullptr;
	if(!Configdlg->Create(IDD_CONFIGDIALOG, pParent))
	{
		LOG_E("窗口创建失败!");
		return nullptr;
	}
	Configdlg->SetParent(pParent);
	return Configdlg;
}

extern"C" DLL_API BOOL destroyWorkDialog(CWorkDialog * dlg)
{
	if (dlg)
	{
		dlg->DestroyWindow();
		delete dlg;
		dlg = nullptr;
	}
	return TRUE;
}

extern"C" DLL_API BOOL destroyConfigDialog(CConfigDialog * dlg)
{
	if (dlg)
	{
		dlg->DestroyWindow();
		delete dlg;
		dlg = nullptr;
	}
	return TRUE;
}
