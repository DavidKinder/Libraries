#pragma once

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

namespace DPI
{
  int getSystemDPI(void);
  int getWindowDPI(CWnd* wnd);
  int getSystemMetrics(int metric, int dpi);
  bool createSystemMenuFont(CFont* font, int dpi);
  void disableDialogResize(CDialog* dlg);

  CRect getMonitorRect(CWnd* wnd);
  CRect getMonitorWorkRect(CWnd* wnd);

  class ContextUnaware
  {
  public:
    ContextUnaware();
    ~ContextUnaware();

  private:
    void* m_context;
  };

  class FontDialog : public CFontDialog
  {
  public:
    FontDialog(LOGFONT* logFont, DWORD flags, CWnd* parentWnd);
	  INT_PTR DoModal();

  protected:
    // The application LOGFONT structure: this is copied before the dialog
    // opens, and updated only if the user closes the dialog by clicking OK.
    LOGFONT* m_appLogFont;
    // The internal LOGFONT structure passed to ChooseFont().
    LOGFONT m_intLogFont;
  };
} // namespace DPI
