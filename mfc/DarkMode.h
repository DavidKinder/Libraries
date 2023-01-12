#pragma once

#define WM_DARKMODE_ACTIVE (WM_APP+1001)

class DarkMode
{
public:
  DarkMode();

  static DarkMode* GetEnabled(void);
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
  CBrush* GetBrush(DarkModeColour colour);
  CPen* GetPen(DarkModeColour colour);

  void DrawSolidBorder(CWnd* wnd, DarkModeColour colour);

protected:
  COLORREF m_colours[Number_Colours];
  CBrush m_brushes[Number_Colours];
  CPen m_pens[Number_Colours];
};

class DarkModeProgressCtrl : public CProgressCtrl
{
public:
  void SetDarkMode(DarkMode* dark);

protected:
  afx_msg void OnNcPaint();
  DECLARE_MESSAGE_MAP()
};

class DarkModeSliderCtrl : public CSliderCtrl
{
protected:
  afx_msg void OnCustomDraw(NMHDR*, LRESULT*);
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  DECLARE_MESSAGE_MAP()
};

class DarkModeToolBar : public CToolBar
{
protected:
  afx_msg void OnCustomDraw(NMHDR*, LRESULT*);
  DECLARE_MESSAGE_MAP()
};
