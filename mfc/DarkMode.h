#pragma once

#define WM_DARKMODE_ACTIVE (WM_APP+1001)

class DarkMode
{
public:
  DarkMode();

  static bool IsEnabled(const char* path);
  static DarkMode* GetEnabled(const char* path);
  static DarkMode* GetActive(CWnd* wnd);

  static void SetAppDarkMode(void);
  static void SetDarkTitle(CWnd* wnd, BOOL dark);
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

  void DrawRectangle(CDC* dc, const CRect& r, DarkColour border, DarkColour fill);
  void DrawControlBorder(CDC* dc, const CRect& r, DarkColour border, DarkColour back, DarkColour fill);
  void DrawButtonBorder(CDC* dc, const CRect& r, DarkColour border, DarkColour back, DarkColour fill);
  void DrawEditBorder(CDC* dc, const CRect& r, DarkColour border, DarkColour back, DarkColour fill, bool focus);

  CRect PrepareNonClientBorder(CWnd* wnd, CWindowDC& dc);
  bool CursorInRect(CWnd* wnd, CRect r);
  DarkColour GetBackground(CWnd* wnd);

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

  BOOL SubclassDlgItem(UINT id, CWnd* parent, UINT imageId);

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
  void SetDarkMode(DarkMode* dark);
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
public:
  void SetDarkMode(DarkMode* dark);

protected:
  afx_msg void OnNcPaint();
  DECLARE_MESSAGE_MAP()
};

class DarkModeProgressCtrl : public CProgressCtrl
{
public:
  virtual void SetDarkMode(DarkMode* dark);

protected:
  afx_msg void OnNcPaint();
  DECLARE_MESSAGE_MAP()
};

class DarkModeTabCtrl : public CTabCtrl
{
protected:
  afx_msg void OnPaint();
  afx_msg void OnMouseMove(UINT nFlags, CPoint point);
  afx_msg LRESULT OnMouseLeave(WPARAM, LPARAM);
  DECLARE_MESSAGE_MAP()

  int m_mouseOverItem = -1;
  bool m_mouseTrack = false;
};

class DarkModePropertyPage : public CPropertyPage
{
  DECLARE_DYNAMIC(DarkModePropertyPage)

public:
  explicit DarkModePropertyPage(UINT id);
  virtual void SetDarkMode(DarkMode* dark, bool init);

protected:
  afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
  DECLARE_MESSAGE_MAP()
};

class DarkModePropertySheet : public CPropertySheet
{
  DECLARE_DYNAMIC(DarkModePropertySheet)

public:
  explicit DarkModePropertySheet(LPCSTR caption);
  virtual void SetDarkMode(DarkMode* dark, bool init);

  virtual BOOL OnInitDialog();

protected:
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  DECLARE_MESSAGE_MAP()

  DarkModeButton m_okBtn;
  DarkModeButton m_cancelBtn;
  DarkModeTabCtrl m_tabCtrl;
};

class DarkModeRadioButton : public CButton
{
public:
  DarkModeRadioButton();
  ~DarkModeRadioButton();

  BOOL SubclassDlgItem(UINT id, CWnd* parent, UINT imageId);

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

class DarkModeStatic : public CStatic
{
protected:
  afx_msg void OnPaint();
  afx_msg void OnEnable(BOOL bEnable);
  DECLARE_MESSAGE_MAP()
};

class DarkModeToolBar : public CToolBar
{
protected:
  afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
  afx_msg void OnCustomDraw(NMHDR*, LRESULT*);
  DECLARE_MESSAGE_MAP()
};
