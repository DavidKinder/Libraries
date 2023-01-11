#pragma once

#define WM_DARKMODE_ACTIVE (WM_APP+1001)

class DarkMode
{
public:
  DarkMode();

  static DarkMode* GetActive(CWnd* wnd);

  static void Set(CFrameWnd* frame, DarkMode* dark);
  static void Set(CReBar* bar, DarkMode* dark);
  static void Set(CControlBar* bar, CReBar* rebar, int index, DarkMode* dark);

  enum DarkModeColour
  {
    Fore = 0,
    Dark1,
    Dark2,
    Dark3,
    Darkest,
    Back,
    Number_Colours
  };

  COLORREF GetColour(DarkModeColour colour);
  CBrush& GetBrush(DarkModeColour colour);

  void DrawSolidBorder(CWnd* wnd, DarkModeColour colour);

protected:
  COLORREF m_colours[Number_Colours];
  CBrush m_brushes[Number_Colours];
};

class DarkModeToolBar : public CToolBar
{
protected: 
  afx_msg void OnCustomDraw(NMHDR*, LRESULT*);
  DECLARE_MESSAGE_MAP()
};
