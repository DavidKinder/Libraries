#include "stdafx.h"
#include "DarkMode.h"
#include "ImagePNG.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DarkMode::DarkMode()
{
  m_colours[Back]    = RGB(0x00,0x00,0x00);
  m_colours[Fore]    = RGB(0xF0,0xF0,0xF0);
  m_colours[Dark1]   = RGB(0x80,0x80,0x80);
  m_colours[Dark2]   = RGB(0x60,0x60,0x60);
  m_colours[Dark3]   = RGB(0x40,0x40,0x40);
  m_colours[Darkest] = RGB(0x20,0x20,0x20);
  m_colours[No_Colour] = 0;
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
  {
    if (colour == No_Colour)
      brush.CreateStockObject(NULL_BRUSH);
    else
      brush.CreateSolidBrush(GetColour(colour));
  }
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

BEGIN_MESSAGE_MAP(DarkModeCheckButton, CButton)
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()

struct DarkModeCheckButton::Impl
{
  ImagePNG check;
};

static const int DarkModeCheckBorder = 3;

DarkModeCheckButton::DarkModeCheckButton()
{
  m_impl = new Impl;
}

DarkModeCheckButton::~DarkModeCheckButton()
{
  delete m_impl;
}

BOOL DarkModeCheckButton::SubclassDlgItem(UINT id, CWnd* parent, UINT imageId)
{
  if (CWnd::SubclassDlgItem(id,parent))
  {
    ImagePNG img;
    if (img.LoadResource(imageId))
    {
      // Get the the size of the check image from the font height
      CDC* dc = GetDC();
      CFont* oldFont = dc->SelectObject(GetFont());
      TEXTMETRIC metrics;
      dc->GetTextMetrics(&metrics);
      dc->SelectObject(oldFont);
      ReleaseDC(dc);
      int imgSize = metrics.tmHeight - (2*DarkModeCheckBorder);

      m_impl->check.Scale(img,CSize(imgSize,imgSize));
      return TRUE;
    }
  }
  return FALSE;
}

void DarkModeCheckButton::OnCustomDraw(NMHDR* nmhdr, LRESULT* result)
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
        // Get the the size of the check box from the font height
        TEXTMETRIC metrics;
        dc->GetTextMetrics(&metrics);
        int btnSize = metrics.tmHeight;
        int y = r.Height()-btnSize;

        // Get the colours for drawing
        DarkMode::DarkColour back = DarkMode::No_Colour;
        DarkMode::DarkColour fore = DarkMode::Fore;
        if (nmcd->uItemState & CDIS_HOT)
          back = DarkMode::Dark2;
        if (nmcd->uItemState & CDIS_SELECTED)
        {
          fore = DarkMode::Dark1;
          back = DarkMode::Dark3;
        }

        // Draw the border and background of the button
        CBrush* oldBrush = dc->SelectObject(dark->GetBrush(back));
        CPen borderPen;
        borderPen.CreatePen(PS_SOLID,1,dark->GetColour(DarkMode::Fore));
        CPen* oldPen = dc->SelectObject(&borderPen);
        CRect btnR(r.TopLeft(),CSize(btnSize,btnSize));
        btnR.OffsetRect(0,y);
        dc->Rectangle(btnR);

        // Draw the check, if needed
        if (GetCheck() == BST_CHECKED)
        {
          ImagePNG image;
          image.Copy(m_impl->check);
          image.Fill(dark->GetColour(fore));
          image.Blend(dark->GetColour(back));
          image.Draw(dc,r.TopLeft()+CPoint(DarkModeCheckBorder,DarkModeCheckBorder));
        }

        // Draw the label
        CString label;
        GetWindowText(label);
        dc->SetTextColor(dark->GetColour(DarkMode::Fore));
        dc->SetBkMode(TRANSPARENT);
        CRect textR(r);
        textR.OffsetRect((3*btnSize)/2,y);
        UINT dtFlags = DT_LEFT|DT_TOP|DT_HIDEPREFIX;
        dc->DrawText(label,textR,dtFlags);

        // Draw the focus rectangle, if needed
        if (CWnd::GetFocus() == this)
        {
          if ((SendMessage(WM_QUERYUISTATE) & UISF_HIDEFOCUS) == 0)
          {
            dc->DrawText(label,textR,dtFlags|DT_CALCRECT);
            textR.InflateRect(2,0);
            dc->SetTextColor(dark->GetColour(DarkMode::Fore));
            dc->SetBkColor(dark->GetColour(DarkMode::Back));
            dc->DrawFocusRect(textR);
          }
        }

        dc->SelectObject(oldPen);
        dc->SelectObject(oldBrush);
      }
      break;
    }
  }
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

BEGIN_MESSAGE_MAP(DarkModeRadioButton, CButton)
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()

void DarkModeRadioButton::OnCustomDraw(NMHDR* nmhdr, LRESULT* result)
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
        // Get the the size of the radio button from the font height
        TEXTMETRIC metrics;
        dc->GetTextMetrics(&metrics);
        int btnSize = metrics.tmHeight;
        int y = r.Height()-btnSize;

        // Get the colours for drawing
        DarkMode::DarkColour back = DarkMode::No_Colour;
        DarkMode::DarkColour fore = DarkMode::Fore;
        if (nmcd->uItemState & CDIS_HOT)
          back = DarkMode::Dark2;
        if (nmcd->uItemState & CDIS_SELECTED)
        {
          fore = DarkMode::Dark1;
          back = DarkMode::Dark3;
        }

        // Draw the border and background of the button
        CBrush* oldBrush = dc->SelectObject(dark->GetBrush(back));
        CPen borderPen;
        borderPen.CreatePen(PS_SOLID,1,dark->GetColour(DarkMode::Fore));
        CPen* oldPen = dc->SelectObject(&borderPen);
        CRect btnR(r.TopLeft(),CSize(btnSize,btnSize));
        btnR.OffsetRect(0,y);
        dc->Rectangle(btnR);

        // Draw the button center, if needed
        if (GetCheck() == BST_CHECKED)
        {
          btnR.DeflateRect(5,5);
          dc->FillSolidRect(btnR,dark->GetColour(fore));
        }

        // Draw the label
        CString label;
        GetWindowText(label);
        dc->SetTextColor(dark->GetColour(DarkMode::Fore));
        dc->SetBkMode(TRANSPARENT);
        CRect textR(r);
        textR.OffsetRect((3*btnSize)/2,y);
        UINT dtFlags = DT_LEFT|DT_TOP|DT_HIDEPREFIX;
        dc->DrawText(label,textR,dtFlags);

        // Draw the focus rectangle, if needed
        if (CWnd::GetFocus() == this)
        {
          if ((SendMessage(WM_QUERYUISTATE) & UISF_HIDEFOCUS) == 0)
          {
            dc->DrawText(label,textR,dtFlags|DT_CALCRECT);
            textR.InflateRect(2,0);
            dc->SetTextColor(dark->GetColour(DarkMode::Fore));
            dc->SetBkColor(dark->GetColour(DarkMode::Back));
            dc->DrawFocusRect(textR);
          }
        }

        dc->SelectObject(oldPen);
        dc->SelectObject(oldBrush);
      }
      break;
    }
  }
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
