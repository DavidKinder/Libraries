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

COLORREF DarkMode::GetColour(DarkModeColour colour)
{
  ASSERT(colour >= 0);
  ASSERT(colour < Number_Colours);

  return m_colours[colour];
}

CBrush* DarkMode::GetBrush(DarkModeColour colour)
{
  ASSERT(colour >= 0);
  ASSERT(colour < Number_Colours);

  CBrush& brush = m_brushes[colour];
  if (brush.GetSafeHandle() == 0)
    brush.CreateSolidBrush(GetColour(colour));
  return &brush;
}

CPen* DarkMode::GetPen(DarkModeColour colour)
{
  ASSERT(colour >= 0);
  ASSERT(colour < Number_Colours);

  CPen& pen = m_pens[colour];
  if (pen.GetSafeHandle() == 0)
    pen.CreatePen(PS_SOLID,1,GetColour(colour));
  return &pen;
}

void DarkMode::DrawSolidBorder(CWnd* wnd, DarkModeColour colour)
{
  CWindowDC dc(wnd);
  CRect rc, rw;
  wnd->GetClientRect(rc);
  wnd->GetWindowRect(rw);
  wnd->ScreenToClient(rw);
  rc.OffsetRect(-rw.TopLeft());
  rw.OffsetRect(-rw.TopLeft());
  dc.ExcludeClipRect(rc);
  dc.IntersectClipRect(rw);
  dc.FillSolidRect(rw,GetColour(colour));
  dc.SelectClipRgn(NULL);
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
    dark->DrawSolidBorder(this,DarkMode::Dark3);
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
          {
            CBrush* oldBrush = dc->SelectObject(dark->GetBrush(DarkMode::Darkest));
            CPen* oldPen = dc->SelectObject(dark->GetPen(DarkMode::Dark3));
            dc->Rectangle(r);
            dc->SelectObject(oldPen);
            dc->SelectObject(oldBrush);
          }
          break;
        case TBCD_THUMB:
          *result = CDRF_SKIPDEFAULT;
          {
            DarkMode::DarkModeColour dmc = DarkMode::Dark2;
            if (nmcd->uItemState & CDIS_SELECTED)
              dmc = DarkMode::Fore;
            else
            {
              // Is the mouse over the thumb?
              CRect scrR(r);
              ClientToScreen(scrR);
              CPoint cursor;
              ::GetCursorPos(&cursor);
              if (scrR.PtInRect(cursor))
                dmc = DarkMode::Dark1;
            }
            dc->FillSolidRect(r,dark->GetColour(dmc));
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
