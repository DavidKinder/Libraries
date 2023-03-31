#include "stdafx.h"
#include "ColourButton.h"
#include "DarkMode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(ColourButton, CButton)
  ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()

ColourButton::ColourButton()
{
  m_colour = RGB(0,0,0);
  m_showDisabled = true;
  m_notifyMsgId = 0;
}

BOOL ColourButton::SubclassDlgItem(UINT id, CWnd* parent, UINT notifyMsgId)
{
  if (CButton::SubclassDlgItem(id,parent))
  {
    m_notifyMsgId = notifyMsgId;
    return TRUE;
  }
  return FALSE;
}

void ColourButton::OnClicked() 
{
  CColorDialog dialog(m_colour,CC_FULLOPEN,this);
  if (dialog.DoModal() == IDOK)
  {
    m_colour = dialog.GetColor();
    Invalidate();
    if (m_notifyMsgId != 0)
      GetParent()->PostMessage(m_notifyMsgId);
  }
}

void ColourButton::OnCustomDraw(NMHDR* nmhdr, LRESULT* result)
{
  NMCUSTOMDRAW* nmcd = (NMCUSTOMDRAW*)nmhdr;
  *result = CDRF_DODEFAULT;

  switch (nmcd->dwDrawStage)
  {
  case CDDS_PREPAINT:
    {
      DarkMode* dark = DarkMode::GetActive(this);
      CDC* dc = CDC::FromHandle(nmcd->hdc);
      CRect r(nmcd->rc);

      bool selected = ((nmcd->uItemState & CDIS_SELECTED) != 0);
      bool disabled = ((nmcd->uItemState & CDIS_DISABLED) != 0);
      bool focus = ((nmcd->uItemState & CDIS_FOCUS) != 0);
      if (SendMessage(WM_QUERYUISTATE) & UISF_HIDEFOCUS)
        focus = false;

      if (dark)
      {
        DarkMode::DarkColour border = DarkMode::Dark2;
        DarkMode::DarkColour fill = DarkMode::Darkest;

        if (GetStyle() & BS_DEFPUSHBUTTON)
          border = DarkMode::Fore;
        if (selected)
          fill = DarkMode::Dark1;
        else if (nmcd->uItemState & CDIS_HOT)
          fill = DarkMode::Dark2;
        if (disabled)
          border = DarkMode::Dark3;
        dark->DrawButtonBorder(dc,r,border,dark->GetBackground(this),fill);
        r.DeflateRect(2,2);
      }
      else if (::IsAppThemed())
      {
        HTHEME theme = ::OpenThemeData(GetSafeHwnd(),L"Button");
        if (theme)
        {
          // Get the area to draw into
          ::GetThemeBackgroundContentRect(theme,
            dc->GetSafeHdc(),BP_PUSHBUTTON,PBS_NORMAL,&(nmcd->rc),r);
          ::CloseThemeData(theme);
        }
      }
      else
      {
        UINT state = DFCS_BUTTONPUSH|DFCS_ADJUSTRECT;
        if (disabled)
          state |= DFCS_INACTIVE;
        if (selected)
          state |= DFCS_PUSHED;
        dc->DrawFrameControl(r,DFC_BUTTON,state);

        if (selected)
          r.OffsetRect(1,1);
      }

      if (!disabled || m_showDisabled)
        DrawControl(dc,r,dark,disabled,focus);
      *result = CDRF_SKIPDEFAULT;
    }
    break;
  }
}

void ColourButton::DrawControl(CDC* dc, CRect r, DarkMode* dark, bool disabled, bool focus)
{
  r.DeflateRect(2,2);

  // Draw the colour that the button represents
  CPen pen;
  if (dark)
    pen.CreatePen(PS_SOLID,1,dark->GetColour(DarkMode::Darkest));
  else
    pen.CreatePen(PS_SOLID,1,::GetSysColor(disabled ? COLOR_GRAYTEXT : COLOR_BTNTEXT));
  CBrush brush;
  brush.CreateSolidBrush(m_colour);
  CPen* oldPen = dc->SelectObject(&pen);
  CBrush* oldBrush = dc->SelectObject(&brush);
  dc->Rectangle(r);
  dc->SelectObject(oldPen);
  dc->SelectObject(oldBrush);

  // Draw the focus rectangle
  if (focus)
  {
    if (dark)
    {
      dc->SetTextColor(dark->GetColour(DarkMode::Fore));
      dc->SetBkColor(dark->GetColour(DarkMode::Back));
    }
    else
    {
      dc->SetTextColor(::GetSysColor(COLOR_BTNTEXT));
      dc->SetBkColor(::GetSysColor(COLOR_WINDOW));
    }
    r.InflateRect(1,1);
    dc->DrawFocusRect(r);
  }
}
