#pragma once
#include "framework.h"
char * CString2C(CString str);

CString C2CString(const char * c);

CString UTF8toUnicode(const char* utf8Str, UINT length);

CString UTF8toUnicode(const char* utf8Str);

CString UnicodeToUTF8(const char* mbcsStr);

CString GetCurPath();