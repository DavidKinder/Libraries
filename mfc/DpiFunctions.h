#pragma once

namespace DPI
{
  int getSystemDPI(void);
  int getWindowDPI(CWnd* wnd);
  int getSystemMetrics(int metric, int dpi);
  bool createSystemMenuFont(CFont* font, int dpi);
  void disableDialogResize(CDialog* dlg);

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
    FontDialog(LPLOGFONT logFont, DWORD flags);
	  INT_PTR DoModal();
  };
} // namespace DPI
