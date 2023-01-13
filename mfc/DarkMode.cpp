#include "stdafx.h"
#include "DarkMode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DarkMode::DarkMode()
{
  m_colours[Back]    = RGB(0x00,0x00,0x00);
  m_colours[Fore]    = RGB(0xe0,0xe0,0xe0);
  m_colours[Dark1]   = RGB(0x80,0x80,0x80);
  m_colours[Dark2]   = RGB(0x60,0x60,0x60);
  m_colours[Dark3]   = RGB(0x40,0x40,0x40);
  m_colours[Darkest] = RGB(0x20,0x20,0x20);
}

DarkMode* DarkMode::GetEnabled(void)
{
  if (false) // Enabled in Windows!
    return new DarkMode();
  return NULL;
}

DarkMode* DarkMode::GetActive(CWnd* wnd)
{
  return (DarkMode*)wnd->GetParentFrame()->SendMessage(WM_DARKMODE_ACTIVE);
}

void DarkMode::Set(CFrameWnd* frame, DarkMode* dark)
{
  if (dark)
  {
    BOOL darkTitle = TRUE;
    ::DwmSetWindowAttribute(frame->GetSafeHwnd(),DWMWA_USE_IMMERSIVE_DARK_MODE,&darkTitle,sizeof darkTitle);
  }

  frame->Invalidate();
}

void DarkMode::Set(CReBar* rebar, DarkMode* dark)
{
  LPCWSTR theme = dark ? L"" : NULL;
  ::SetWindowTheme(rebar->GetSafeHwnd(),theme,theme);

  if (dark)
    rebar->GetReBarCtrl().SetBkColor(dark->GetColour(DarkMode::Darkest));
}

void DarkMode::Set(CControlBar* bar, CReBar* rebar, int index, DarkMode* dark)
{
  LPCWSTR theme = dark ? L"" : NULL;
  ::SetWindowTheme(bar->GetSafeHwnd(),theme,theme);

  if (dark)
  {
    REBARBANDINFO bandInfo = { sizeof(REBARBANDINFO),0 };
    bandInfo.fMask = RBBIM_COLORS;
    bandInfo.clrFore = dark->GetColour(DarkMode::Fore);
    bandInfo.clrBack = dark->GetColour(DarkMode::Darkest);
    rebar->GetReBarCtrl().SetBandInfo(index,&bandInfo);
  }
}

COLORREF DarkMode::GetColour(DarkColour colour)
{
  ASSERT(colour >= 0);
  ASSERT(colour < Number_Colours);

  return m_colours[colour];
}

CBrush* DarkMode::GetBrush(DarkColour colour)
{
  ASSERT(colour >= 0);
  ASSERT(colour < Number_Colours);

  CBrush& brush = m_brushes[colour];
  if (brush.GetSafeHandle() == 0)
    brush.CreateSolidBrush(GetColour(colour));
  return &brush;
}

void DarkMode::DrawBorder(CDC* dc, const CRect& r, DarkColour border, DarkColour fill)
{
  CPen pen;
  pen.CreatePen(PS_SOLID,1,GetColour(border));

  CBrush* oldBrush = dc->SelectObject(GetBrush(fill));
  CPen* oldPen = dc->SelectObject(&pen);
  dc->Rectangle(r);
  dc->SelectObject(oldPen);
  dc->SelectObject(oldBrush);
}

void DarkMode::DrawNonClientBorder(CWnd* wnd, DarkColour border, DarkColour fill)
{
  CWindowDC dc(wnd);

  // Get the window and client rectangles, in the window co-ordinate space
  CRect rw, rc;
  wnd->GetWindowRect(rw);
  wnd->ScreenToClient(rw);
  wnd->GetClientRect(rc);
  rc.OffsetRect(-rw.TopLeft());
  rw.OffsetRect(-rw.TopLeft());

  // Make the clipping region the window rectangle, excluding the client rectangle
  dc.ExcludeClipRect(rc);
  dc.IntersectClipRect(rw);

  // Draw the non-client border
  DrawBorder(&dc,rw,border,fill);

  // Reset the clipping region
  dc.SelectClipRgn(NULL);
}

BEGIN_MESSAGE_MAP(DarkModeButton, CButton)
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()

void DarkModeButton::OnCustomDraw(NMHDR* nmhdr, LRESULT* result)
{
  NMCUSTOMDRAW* nmcd = (NMCUSTOMDRAW*)nmhdr;
  *result = CDRF_DODEFAULT;

  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    CDC* dc = CDC::FromHandle(nmcd->hdc);
    CRect r(nmcd->rc);

    switch (nmcd->dwDrawStage)
    {
    case CDDS_PREERASE:
    case CDDS_PREPAINT:
      *result = CDRF_SKIPDEFAULT;
      {
        DarkMode::DarkColour border = DarkMode::Dark2;
        DarkMode::DarkColour fill = DarkMode::Darkest;
        DarkMode::DarkColour text = DarkMode::Fore;

        if (nmcd->uItemState & CDIS_SELECTED)
          fill = DarkMode::Dark1;
        else if (nmcd->uItemState & CDIS_HOT)
          fill = DarkMode::Dark2;
        else if (nmcd->uItemState & CDIS_DISABLED)
        {
          border = DarkMode::Dark3;
          text = DarkMode::Dark3;
        }
        dark->DrawBorder(dc,r,border,fill);

        CString label;
        GetWindowText(label);
        dc->SetTextColor(dark->GetColour(text));
        dc->SetBkMode(TRANSPARENT);
        dc->DrawText(label,r,DT_CENTER|DT_VCENTER|DT_HIDEPREFIX|DT_SINGLELINE);
      }
      break;
    }
  }
}

BEGIN_MESSAGE_MAP(DarkModeProgressCtrl, CProgressCtrl)
  ON_WM_NCPAINT()
END_MESSAGE_MAP()

void DarkModeProgressCtrl::SetDarkMode(DarkMode* dark)
{
  LPCWSTR theme = dark ? L"" : NULL;
  ::SetWindowTheme(GetSafeHwnd(),theme,theme);

  if (dark)
  {
    SetBkColor(dark->GetColour(DarkMode::Back));
    SetBarColor(dark->GetColour(DarkMode::Dark1));
  }
}

void DarkModeProgressCtrl::OnNcPaint()
{
  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
    dark->DrawNonClientBorder(this,DarkMode::Dark3,DarkMode::Darkest);
  else
    Default();
}

BEGIN_MESSAGE_MAP(DarkModeSliderCtrl, CSliderCtrl)
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
  ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void DarkModeSliderCtrl::OnCustomDraw(NMHDR* nmhdr, LRESULT* result)
{
  *result = CDRF_DODEFAULT;

  NMCUSTOMDRAW* nmcd = (NMCUSTOMDRAW*)nmhdr;
  switch (nmcd->dwDrawStage)
  {
  case CDDS_PREPAINT:
    *result = CDRF_NOTIFYITEMDRAW;
    break;
  case CDDS_ITEMPREPAINT:
    {
      DarkMode* dark = DarkMode::GetActive(this);
      if (dark)
      {
        CDC* dc = CDC::FromHandle(nmcd->hdc);
        CRect r(nmcd->rc);

        switch (nmcd->dwItemSpec)
        {
        case TBCD_CHANNEL:
          *result = CDRF_SKIPDEFAULT;
          dark->DrawBorder(dc,r,DarkMode::Dark3,DarkMode::Darkest);
          break;
        case TBCD_THUMB:
          *result = CDRF_SKIPDEFAULT;
          {
            DarkMode::DarkColour thumb = DarkMode::Dark2;
            if (nmcd->uItemState & CDIS_SELECTED)
              thumb = DarkMode::Fore;
            else
            {
              // Is the mouse over the thumb?
              CRect scrR(r);
              ClientToScreen(scrR);
              CPoint cursor;
              ::GetCursorPos(&cursor);
              if (scrR.PtInRect(cursor))
                thumb = DarkMode::Dark1;
            }
            dc->FillSolidRect(r,dark->GetColour(thumb));
          }
          break;
        case TBCD_TICS:
          *result = CDRF_SKIPDEFAULT;
          break;
        }
      }
    }
    break;
  }
}

BOOL DarkModeSliderCtrl::OnEraseBkgnd(CDC* pDC)
{
  return TRUE;
}

BEGIN_MESSAGE_MAP(DarkModeToolBar, CToolBar)
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()

void DarkModeToolBar::OnCustomDraw(NMHDR* nmhdr, LRESULT* result)
{
  NMTBCUSTOMDRAW* nmtbcd = (NMTBCUSTOMDRAW*)nmhdr;
  *result = CDRF_DODEFAULT;

  switch (nmtbcd->nmcd.dwDrawStage)
  {
  case CDDS_PREPAINT:
    *result = CDRF_NOTIFYITEMDRAW;
    break;
  case CDDS_ITEMPREPAINT:
    {
      DarkMode* dark = DarkMode::GetActive(this);
      if (dark)
      {
        *result = TBCDRF_NOEDGES|TBCDRF_NOETCHEDEFFECT|TBCDRF_HILITEHOTTRACK;
        nmtbcd->clrHighlightHotTrack = dark->GetColour(DarkMode::Dark2);
        nmtbcd->clrText = dark->GetColour(DarkMode::Fore);
      }
    }
    break;
  }
}
