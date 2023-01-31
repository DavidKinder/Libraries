#pragma once

#define WM_DARKMODE_ACTIVE (WM_APP+1001)

class DarkMode
{
public:
  DarkMode();

  static bool IsEnabled(void);
  static DarkMode* GetEnabled(void);
  static DarkMode* GetActive(CWnd* wnd);

  static void Set(CFrameWnd* frame, DarkMode* dark);
  static void Set(CDialog* dlg, DarkMode* dark);
  static void Set(CReBar* bar, DarkMode* dark);
  static void Set(CControlBar* bar, CReBar* rebar, int index, DarkMode* dark);
  static void Set(CToolTipCtrl* tip, DarkMode* dark);

  enum DarkColour
  {
    Fore = 0,
    Dark1,
    Dark2,
    Dark3,
    Darkest,
    Back,
    No_Colour,
    Number_Colours
  };

  COLORREF GetColour(DarkColour colour);
  CBrush* GetBrush(DarkColour colour);

  void DrawBorder(CDC* dc, const CRect& r, DarkColour border, DarkColour fill);
  void DrawNonClientBorder(CWnd* wnd, DarkColour colour, DarkColour fill);

  bool CursorInRect(CWnd* wnd, CRect r);

protected:
  COLORREF m_colours[Number_Colours];
  CBrush m_brushes[Number_Colours];
};

class DarkModeButton : public CButton
{
protected:
  afx_msg void OnCustomDraw(NMHDR*, LRESULT*);
  DECLARE_MESSAGE_MAP()
};

class DarkModeCheckButton : public CButton
{
  DECLARE_DYNAMIC(DarkModeCheckButton)

public:
  DarkModeCheckButton();
  ~DarkModeCheckButton();

  BOOL SubclassDlgItem(UINT id, CWnd* parent, UINT imageId, DarkMode::DarkColour back);

protected:
  afx_msg void OnCustomDraw(NMHDR*, LRESULT*);
  DECLARE_MESSAGE_MAP()

  virtual bool HasFocusRect(UINT uiState);

  struct Impl;
  Impl* m_impl;
};

class DarkModeComboBox : public CComboBox
{
public:
  void SetDarkBorder(DarkMode::DarkColour colour, DarkMode::DarkColour activeColour);

protected:
  afx_msg void OnPaint();
  afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
  afx_msg LRESULT OnSetCurSel(WPARAM, LPARAM);
  DECLARE_MESSAGE_MAP()

  DarkMode::DarkColour m_border = DarkMode::Dark1;
  DarkMode::DarkColour m_activeBorder = DarkMode::Fore;
};

class DarkModeEdit : public CEdit
{
  DECLARE_DYNAMIC(DarkModeEdit)

protected:
  afx_msg void OnNcPaint();
  DECLARE_MESSAGE_MAP()
};

class DarkModeGroupBox : public CButton
{
protected:
  afx_msg void OnPaint();
  DECLARE_MESSAGE_MAP()
};

class DarkModeListBox : public CListBox
{
protected:
  afx_msg void OnNcPaint();
  DECLARE_MESSAGE_MAP()
};

class DarkModeProgressCtrl : public CProgressCtrl
{
public:
  void SetDarkMode(DarkMode* dark);

protected:
  afx_msg void OnNcPaint();
  DECLARE_MESSAGE_MAP()
};

class DarkModePropertyPage : public CPropertyPage
{
  DECLARE_DYNAMIC(DarkModePropertyPage)

public:
  DarkModePropertyPage(UINT id);

protected:
  afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
  DECLARE_MESSAGE_MAP()
};

class DarkModeRadioButton : public CButton
{
public:
  DarkModeRadioButton();
  ~DarkModeRadioButton();

  BOOL SubclassDlgItem(UINT id, CWnd* parent, UINT imageId, DarkMode::DarkColour back);

protected:
  afx_msg void OnCustomDraw(NMHDR*, LRESULT*);
  DECLARE_MESSAGE_MAP()

  struct Impl;
  Impl* m_impl;
};

class DarkModeSliderCtrl : public CSliderCtrl
{
protected:
  afx_msg void OnCustomDraw(NMHDR*, LRESULT*);
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg LRESULT OnSetPos(WPARAM, LPARAM);
  DECLARE_MESSAGE_MAP()
};

class DarkModeToolBar : public CToolBar
{
protected:
  afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
  afx_msg void OnCustomDraw(NMHDR*, LRESULT*);
  DECLARE_MESSAGE_MAP()
};
