#include "StdAfx.h"
#include "DpiFunctions.h"

namespace {
  HMODULE getUser32(void)
  {
    static HMODULE user = 0;

    if (user == 0)
      user = ::LoadLibrary("user32.dll");
    return user;
  }

  VOID* setContext(VOID* context)
  {
    typedef VOID*(__stdcall *PFNSETTHREADDPIAWARENESSCONTEXT)(VOID*);

    HMODULE user = getUser32();
    PFNSETTHREADDPIAWARENESSCONTEXT setThreadDpiAwarenessContext = (PFNSETTHREADDPIAWARENESSCONTEXT)
      ::GetProcAddress(user,"SetThreadDpiAwarenessContext");
    if (setThreadDpiAwarenessContext != NULL)
      return (*setThreadDpiAwarenessContext)(context);
    return 0;
  }

  VOID* getMonitorFromWindow(CWnd* wnd)
  {
    typedef VOID*(__stdcall *PFNMONITORFROMWINDOW)(HWND, DWORD);

    HMODULE user = getUser32();
    PFNMONITORFROMWINDOW monitorFromWindow = (PFNMONITORFROMWINDOW)
      ::GetProcAddress(user,"MonitorFromWindow");
    if (monitorFromWindow != NULL)
      return (*monitorFromWindow)(wnd->GetSafeHwnd(), MONITOR_DEFAULTTOPRIMARY);
    return 0;
  }

  BOOL getMonitorInfo(VOID* monitor, MONITORINFO* monInfo)
  {
    typedef BOOL(__stdcall *PFNGETMONITORINFO)(VOID*, MONITORINFO*);

    HMODULE user = getUser32();
    PFNGETMONITORINFO getMonitorInfo = (PFNGETMONITORINFO)
      ::GetProcAddress(user,"GetMonitorInfoA");
    if (getMonitorInfo != NULL)
      return (*getMonitorInfo)(monitor,monInfo);
    return FALSE;
  }

} // unnamed namespace

int DPI::getSystemDPI(void)
{
  typedef UINT(__stdcall *PFNGETDPIFORSYSTEM)(VOID);

  HMODULE user = getUser32();
  PFNGETDPIFORSYSTEM getDpiForSystem = (PFNGETDPIFORSYSTEM)
    ::GetProcAddress(user,"GetDpiForSystem");
  if (getDpiForSystem != NULL)
    return (*getDpiForSystem)();

  HDC dc = ::GetDC(0);
  int dpi = ::GetDeviceCaps(dc,LOGPIXELSY);
  ::ReleaseDC(0,dc);
  return dpi;
}

int DPI::getWindowDPI(CWnd* wnd)
{
  ASSERT(wnd->GetSafeHwnd());

  typedef UINT(__stdcall *PFNGETDPIFORWINDOW)(HWND);

  HMODULE user = getUser32();
  PFNGETDPIFORWINDOW getDpiForWindow = (PFNGETDPIFORWINDOW)
    ::GetProcAddress(user,"GetDpiForWindow");
  if (getDpiForWindow != NULL)
    return (*getDpiForWindow)(wnd->GetSafeHwnd());

  CDC* dc = wnd->GetDC();
  int dpi = dc->GetDeviceCaps(LOGPIXELSY);
  wnd->ReleaseDC(dc);
  return dpi;
}

int DPI::getSystemMetrics(int metric, int dpi)
{
  typedef int(__stdcall *PFNGETSYSTEMMETRICSFORDPI)(int, UINT);

  HMODULE user = getUser32();
  PFNGETSYSTEMMETRICSFORDPI getSystemMetricsForDpi = (PFNGETSYSTEMMETRICSFORDPI)
    ::GetProcAddress(user,"GetSystemMetricsForDpi");
  if (getSystemMetricsForDpi != NULL)
    return (*getSystemMetricsForDpi)(metric,dpi);

  return ::GetSystemMetrics(metric);
}

// SystemParametersInfoForDpi() requires this version of NONCLIENTMETRICSW
typedef struct _NONCLIENTMETRICSW_0600
{
  UINT cbSize;
  int iBorderWidth;
  int iScrollWidth;
  int iScrollHeight;
  int iCaptionWidth;
  int iCaptionHeight;
  LOGFONTW lfCaptionFont;
  int iSmCaptionWidth;
  int iSmCaptionHeight;
  LOGFONTW lfSmCaptionFont;
  int iMenuWidth;
  int iMenuHeight;
  LOGFONTW lfMenuFont;
  LOGFONTW lfStatusFont;
  LOGFONTW lfMessageFont;
  int iPaddedBorderWidth;
}
NONCLIENTMETRICSW_0600;

bool DPI::createSystemMenuFont(CFont* font, int dpi, double scale)
{
  typedef BOOL(__stdcall *PFNSYSPARAMINFOFORDPI)(UINT, UINT, PVOID, UINT, UINT);

  HMODULE user = getUser32();
  PFNSYSPARAMINFOFORDPI sysParamInfoForDpi = (PFNSYSPARAMINFOFORDPI)
    ::GetProcAddress(user,"SystemParametersInfoForDpi");
  if (sysParamInfoForDpi != NULL)
  {
    NONCLIENTMETRICSW_0600 ncm = { 0 };
    ncm.cbSize = sizeof ncm;
    if ((*sysParamInfoForDpi)(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0,dpi))
    {
      ncm.lfMenuFont.lfHeight = (int)(scale*ncm.lfMenuFont.lfHeight);
      HFONT fontHandle = ::CreateFontIndirectW(&ncm.lfMenuFont);
      if (fontHandle)
      {
        font->Attach(fontHandle);
        return true;
      }
    }
  }

  {
    NONCLIENTMETRICS ncm = { 0 };
    ncm.cbSize = sizeof ncm;
    if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0))
    {
      ncm.lfMenuFont.lfHeight = (int)(scale*ncm.lfMenuFont.lfHeight);
      if (font->CreateFontIndirect(&ncm.lfMenuFont))
        return true;
    }
  }

  return false;
}

void DPI::disableDialogDPI(CWnd* dlg)
{
  typedef BOOL(__stdcall *PFNSETDIALOGDPICHANGEBEHAVIOR)(HWND, int, int);

  HMODULE user = getUser32();
  PFNSETDIALOGDPICHANGEBEHAVIOR setDialogDpiChangeBehavior = (PFNSETDIALOGDPICHANGEBEHAVIOR)
    ::GetProcAddress(user,"SetDialogDpiChangeBehavior");
  if (setDialogDpiChangeBehavior != NULL)
    (*setDialogDpiChangeBehavior)(dlg->GetSafeHwnd(),1,1); // Set DDC_DISABLE_ALL
}

CRect DPI::getMonitorRect(CWnd* wnd)
{
  VOID* monitor = getMonitorFromWindow(wnd);
  if (monitor != 0)
  {
    MONITORINFO monInfo = { 0 };
    monInfo.cbSize = sizeof monInfo;
    if (getMonitorInfo(monitor,&monInfo))
      return monInfo.rcMonitor;
  }
  return CRect(0,0,::GetSystemMetrics(SM_CXSCREEN),::GetSystemMetrics(SM_CYSCREEN));
}

CRect DPI::getMonitorWorkRect(CWnd* wnd)
{
  VOID* monitor = getMonitorFromWindow(wnd);
  if (monitor != 0)
  {
    MONITORINFO monInfo = { 0 };
    monInfo.cbSize = sizeof monInfo;
    if (getMonitorInfo(monitor,&monInfo))
      return monInfo.rcWork;
  }

  CRect workArea;
  if (SystemParametersInfo(SPI_GETWORKAREA,0,&workArea,0) != FALSE)
    return workArea;
  return CRect(0,0,::GetSystemMetrics(SM_CXSCREEN),::GetSystemMetrics(SM_CYSCREEN));
}

DPI::ContextUnaware::ContextUnaware()
{
  // DPI_AWARENESS_CONTEXT_UNAWARE
  m_context = setContext((VOID*)-1); 
}

DPI::ContextUnaware::~ContextUnaware()
{
   setContext(m_context);
}

DPI::ContextSystem::ContextSystem()
{
  // DPI_AWARENESS_CONTEXT_SYSTEM_AWARE
  m_context = setContext((VOID*)-2); 
}

DPI::ContextSystem::~ContextSystem()
{
   setContext(m_context);
}

DPI::FontDialog::FontDialog(LOGFONT* logFont, DWORD flags, CWnd* parentWnd)
  : CFontDialog(&m_intLogFont,flags,NULL,parentWnd), m_appLogFont(logFont)
{
}

INT_PTR DPI::FontDialog::DoModal()
{
  // Copy the application LOGFONT to the internal LOGFONT
  memcpy(&m_intLogFont,m_appLogFont,sizeof(LOGFONT));

  // Adjust the font height to be relative to the system DPI, not the window DPI
  int dpi1 = DPI::getWindowDPI(m_pParentWnd);
  m_intLogFont.lfHeight = MulDiv(m_intLogFont.lfHeight,DPI::getSystemDPI(),dpi1);

  // Show the font dialog using the system DPI context
  void* context = setContext((VOID*)-2); // DPI_AWARENESS_CONTEXT_SYSTEM_AWARE
  INT_PTR result = CFontDialog::DoModal();
  setContext(context);

  if (result == IDOK)
  {
    // Copy the internal LOGFONT to the application LOGFONT
    memcpy(m_appLogFont,&m_intLogFont,sizeof(LOGFONT));

    // Get the window DPI again, as it might have changed while the dialog was open
    int dpi2 = DPI::getWindowDPI(m_pParentWnd);

    // Set the new font height relative to the current window DPI, not the system DPI
    m_appLogFont->lfHeight = -MulDiv(m_cf.iPointSize,dpi2,720);
  }
  return result;
}
