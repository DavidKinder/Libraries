#pragma once

class ColourButton : public CButton
{
public:
  ColourButton();

  COLORREF GetCurrentColour(void) { return m_colour; }
  void SetCurrentColour(COLORREF c) { m_colour = c; }

  void SetShowDisabled(bool show) { m_showDisabled = show; }

  BOOL SubclassDlgItem(UINT id, CWnd* parent, UINT notifyMsgId = 0);

protected:
  afx_msg void OnClicked();
  afx_msg void OnCustomDraw(NMHDR*, LRESULT*);

  void DrawControl(CDC* dc, CRect r, bool disabled, bool focus);

  DECLARE_MESSAGE_MAP()

  COLORREF m_colour;
  bool m_showDisabled;
  UINT m_notifyMsgId;
};
