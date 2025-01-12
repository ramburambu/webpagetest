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

class CxImage;

class CapturedImage {
public:
  typedef enum {
    VIDEO,
    START_RENDER,
    DOCUMENT_COMPLETE,
    FULLY_LOADED,
    RESPONSIVE_CHECK,
    RESULT,
    UNKNOWN
  } TYPE;

  CapturedImage();
  CapturedImage(const CapturedImage& src){*this = src;}
  CapturedImage(HWND wnd, TYPE type = UNKNOWN, RECT * rect = NULL);
  ~CapturedImage();
  const CapturedImage& operator =(const CapturedImage& src);
  void Free();
  bool Get(CxImage& image);

  HBITMAP       _bitmap_handle;
  LARGE_INTEGER _capture_time;
  TYPE          _type;
};

class ScreenCapture {
public:
  ScreenCapture();
  ~ScreenCapture(void);
  void Capture(HWND wnd, CapturedImage::TYPE type, bool crop_viewport = true);
  CapturedImage CaptureImage(HWND wnd, 
                    CapturedImage::TYPE type = CapturedImage::UNKNOWN,
                    bool crop_viewport = true);
  bool GetImage(CapturedImage::TYPE type, CxImage& image);
  void Lock();
  void Unlock();
  void Reset();
  void SetViewport(RECT& viewport);
  void ClearViewport();
  bool IsViewportSet();

  CAtlList<CapturedImage> _captured_images;
  RECT _viewport;

private:
  CRITICAL_SECTION cs;
  bool _viewport_set;
};
