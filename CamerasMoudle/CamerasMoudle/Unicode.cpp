#pragma once
#include "Unicode.h"
char * CString2C(CString str)
{
	int length = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	char* pTemp = new char[length];
	WideCharToMultiByte(CP_ACP, 0, str, -1, pTemp, length, NULL, NULL);
	return pTemp;
}

CString C2CString(const char * c)
{
	return CString(c);
}

CString UTF8toUnicode(const char* utf8Str, UINT length)
{
	CString unicodeStr;
	unicodeStr = _T("");

	if (!utf8Str)
		return unicodeStr;

	if (length == 0)
		return unicodeStr;


	WCHAR chr = 0;//一个中文字符
	for (UINT i = 0; i < length;)
	{
		//UTF8的三种中文格式
		if ((0x80 & utf8Str[i]) == 0) //只占用一个字节
		{
			chr = utf8Str[i];
			i++;
		}
		else if ((0xE0 & utf8Str[i]) == 0xC0) //占用两个字节
		{
			chr = (utf8Str[i + 0] & 0x3F) << 6;
			chr |= (utf8Str[i + 1] & 0x3F);
			i += 2;
		}
		else if ((0xF0 & utf8Str[i]) == 0xE0)//占用三个字节
		{
			chr = (utf8Str[i + 0] & 0x1F) << 12;
			chr |= (utf8Str[i + 1] & 0x3F) << 6;
			chr |= (utf8Str[i + 2] & 0x3F);
			i += 3;
		}

		else
		{
			return unicodeStr;
		}
		unicodeStr.AppendChar(chr);
	}

	return unicodeStr;
}

CString UTF8toUnicode(const char* utf8Str)
{
	UINT theLength = strlen(utf8Str);
	return UTF8toUnicode(utf8Str, theLength);
}



CString UnicodeToUTF8(const char* mbcsStr)
{
	wchar_t*  wideStr;
	char*   utf8Str;
	int   charLen;
	charLen = MultiByteToWideChar(CP_UTF8, 0, mbcsStr, -1, NULL, 0);
	wideStr = (wchar_t*)malloc(sizeof(wchar_t)*charLen);
	MultiByteToWideChar(CP_ACP, 0, mbcsStr, -1, wideStr, charLen);


	charLen = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, NULL, 0, NULL, NULL);


	utf8Str = (char*)malloc(charLen);


	WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, utf8Str, charLen, NULL, NULL);


	free(wideStr);
	return C2CString(utf8Str);
}

CString GetCurPath()
{
	TCHAR _szPath[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, _szPath, MAX_PATH);
	(_tcsrchr(_szPath, _T('\\')))[1] = 0;//删除文件名，只获得路径 字串
	CString strPath;
	for (int n = 0; _szPath[n]; n++)
	{
		if (_szPath[n] != _T('\\'))
		{
			strPath += _szPath[n];
		}
		else
		{
			strPath += _T("\\\\");
		}
	}
	return strPath;
}
