#pragma once
#include "CTestDialog.h"
#include "CConfigDialog.h"


#ifdef DLL_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif 

extern"C" DLL_API ModInfo* getModInfo(CConfigDialog* dlg);                        // 测试项信息导出

extern"C" DLL_API CWorkDialog * getWorkDialog(CWnd * pParent);                    // 测试项窗口导出

extern"C" DLL_API CConfigDialog * getConfigDialog(CWnd * pParent);                // 配置窗口导出

extern "C" DLL_API BOOL destroyWorkDialog(CWorkDialog* dlg);                      // 销毁测试项工作窗口

extern "C" DLL_API BOOL destroyConfigDialog(CConfigDialog* dlg);                  // 销毁配置窗口


