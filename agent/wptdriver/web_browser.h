/******************************************************************************
Copyright (c) 2010, Google Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of the <ORGANIZATION> nor the names of its contributors 
    may be used to endorse or promote products derived from this software 
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#pragma once
#include "ipfw.h"

class BrowserSettings;

class WebBrowser {
public:
  WebBrowser(WptSettings& settings, WptTestDriver& test, WptStatus &status, 
             BrowserSettings& browser, CIpfw &ipfw);
  ~WebBrowser(void);

  bool RunAndWait();
  void ClearUserData();

private:
  void InjectDll();
  bool FindBrowserChild(DWORD pid, PROCESS_INFORMATION& pi,
                        LPCTSTR browser_exe);
  void ConfigureFirefoxPrefs();
  void ConfigureIESettings();
  HANDLE FindAdditionalHookProcess(HANDLE launched_process, CString exe);

  WptSettings&    _settings;
  WptTestDriver&  _test;
  WptStatus&      _status;
  BrowserSettings& _browser;
  CIpfw&          _ipfw;

  HANDLE        _browser_process;
  HANDLE  _browser_started_event;
  HANDLE  _browser_done_event;

  CRITICAL_SECTION  cs;
  SECURITY_ATTRIBUTES null_dacl;
  SECURITY_DESCRIPTOR SD;
};
