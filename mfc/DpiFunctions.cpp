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

bool DPI::createSystemMenuFont(CFont* font, int dpi)
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
      if (font->CreateFontIndirect(&ncm.lfMenuFont))
        return true;
    }
  }

  return false;
}

void DPI::disableDialogResize(CDialog* dlg)
{
  typedef BOOL(__stdcall *PFNSETDIALOGDPICHANGEBEHAVIOR)(HWND, int, int);

  HMODULE user = getUser32();
  PFNSETDIALOGDPICHANGEBEHAVIOR setDialogDpiChangeBehavior = (PFNSETDIALOGDPICHANGEBEHAVIOR)
    ::GetProcAddress(user,"SetDialogDpiChangeBehavior");
  if (setDialogDpiChangeBehavior != NULL)
    (*setDialogDpiChangeBehavior)(dlg->GetSafeHwnd(),2,2); // Set DDC_DISABLE_RESIZE
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

DPI::FontDialog::FontDialog(LPLOGFONT logFont, DWORD flags) : CFontDialog(logFont,flags)
{
}

INT_PTR DPI::FontDialog::DoModal()
{
  int dpi = DPI::getWindowDPI(AfxGetMainWnd());
  LONG lfHeight = m_cf.lpLogFont->lfHeight;
  m_cf.lpLogFont->lfHeight = MulDiv(lfHeight,DPI::getSystemDPI(),dpi);

  // DPI_AWARENESS_CONTEXT_SYSTEM_AWARE
  void* context = setContext((VOID*)-2);

  INT_PTR result = CFontDialog::DoModal();
  setContext(context);

  if (result == IDOK)
    m_cf.lpLogFont->lfHeight = -MulDiv(m_cf.iPointSize,dpi,720);
  else
    m_cf.lpLogFont->lfHeight = lfHeight;
  return result;
}
