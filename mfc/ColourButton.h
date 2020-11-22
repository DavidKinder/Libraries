#pragma once

class ColourButton : public CButton
{
public:
  ColourButton();

  COLORREF GetCurrentColour(void) { return m_colour; }
  void SetCurrentColour(COLORREF c) { m_colour = c; }

  BOOL SubclassDlgItem(UINT id, CWnd* parent, UINT notifyMsgId = 0);

protected:
  afx_msg void OnClicked();
  afx_msg void OnCustomDraw(NMHDR*, LRESULT*);

  void DrawControl(CDC* dc, CRect r, bool disabled, bool focus);

  DECLARE_MESSAGE_MAP()

  COLORREF m_colour;
  UINT m_notifyMsgId;
};
