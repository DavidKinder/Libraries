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

bool DarkMode::IsEnabled(void)
{
  CRegKey key;
  LPCSTR path = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
  if (key.Open(HKEY_CURRENT_USER,path,KEY_READ) == ERROR_SUCCESS)
  {
    DWORD theme = 0;
    if (key.QueryDWORDValue("AppsUseLightTheme",theme) == ERROR_SUCCESS)
    {
      if (theme == 0)
        return true;
    }
  }
  return false;
}

DarkMode* DarkMode::GetEnabled(void)
{
  if (IsEnabled())
    return new DarkMode();
  return NULL;
}

DarkMode* DarkMode::GetActive(CWnd* wnd)
{
  CWnd* frame = wnd->IsFrameWnd() ? wnd : wnd->GetParentFrame();
  return (DarkMode*)frame->SendMessage(WM_DARKMODE_ACTIVE);
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

void DarkMode::Set(CDialog* dlg, DarkMode* dark)
{
  if (dark)
  {
    BOOL darkTitle = TRUE;
    ::DwmSetWindowAttribute(dlg->GetSafeHwnd(),DWMWA_USE_IMMERSIVE_DARK_MODE,&darkTitle,sizeof darkTitle);
  }

  dlg->Invalidate();
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

void DarkMode::Set(CToolTipCtrl* tip, DarkMode* dark)
{
  HTHEME theme = ::GetWindowTheme(tip->GetSafeHwnd());

  if (dark)
  {
    // Remove any theme and set the colours
    if (theme != 0)
      ::SetWindowTheme(tip->GetSafeHwnd(),L"",L"");
    tip->SetTipBkColor(dark->GetColour(DarkMode::Dark3));
    tip->SetTipTextColor(dark->GetColour(DarkMode::Fore));
  }
  else
  {
    // Set the theme back, if needed
    if (theme == 0)
      ::SetWindowTheme(tip->GetSafeHwnd(),NULL,NULL);
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

bool DarkMode::CursorInRect(CWnd* wnd, CRect r)
{
  CPoint cursor;
  ::GetCursorPos(&cursor);
  wnd->ClientToScreen(r);
  return r.PtInRect(cursor);
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
        CFont* oldFont = dc->SelectObject(GetFont());
        dc->DrawText(label,r,DT_CENTER|DT_VCENTER|DT_HIDEPREFIX|DT_SINGLELINE);

        dc->SelectObject(oldFont);
      }
      break;
    }
  }
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
        // Get the the size of the check box
        int btnSize = m_impl->check.Size().cy + (2*DarkModeCheckBorder);
        int btnY = (r.Height()-btnSize)/2;

        // Get the colours for drawing
        DarkMode::DarkColour back = DarkMode::No_Colour;
        DarkMode::DarkColour fore = DarkMode::Dark1;
        if (nmcd->uItemState & CDIS_HOT)
          fore = DarkMode::Fore;
        if (nmcd->uItemState & CDIS_SELECTED)
        {
          fore = DarkMode::Fore;
          back = DarkMode::Dark3;
        }

        // Draw the border and background of the button
        CRect btnR(r.TopLeft(),CSize(btnSize,btnSize));
        btnR.OffsetRect(0,btnY);
        dark->DrawBorder(dc,btnR,fore,back);

        // Draw the check, if needed
        if (GetCheck() == BST_CHECKED)
        {
          ImagePNG image;
          image.Copy(m_impl->check);
          image.Fill(dark->GetColour(fore));
          image.Blend(dark->GetColour(back));
          image.Draw(dc,r.TopLeft()+CPoint(DarkModeCheckBorder,DarkModeCheckBorder));
        }

        // Get the bounding rectangle for the label
        CFont* oldFont = dc->SelectObject(GetFont());
        TEXTMETRIC metrics;
        dc->GetTextMetrics(&metrics);
        int textY = (r.Height()-metrics.tmHeight)/2;
        CRect textR(r);
        textR.OffsetRect((3*btnSize)/2,textY);

        // Draw the label
        CString label;
        GetWindowText(label);
        dc->SetTextColor(dark->GetColour(DarkMode::Fore));
        dc->SetBkMode(TRANSPARENT);
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

        dc->SelectObject(oldFont);
      }
      break;
    }
  }
}

BEGIN_MESSAGE_MAP(DarkModeComboBox, CComboBox)
  ON_WM_PAINT()
END_MESSAGE_MAP()

void DarkModeComboBox::SetDarkBorder(DarkMode::DarkColour colour, DarkMode::DarkColour activeColour)
{
  m_border = colour;
  m_activeBorder = activeColour;
}

int DarkModeComboBox::SetCurSel(int select)
{
  int result = CComboBox::SetCurSel(select);

  // Changing the combo box selection will cause the internal painting logic
  // of the control to be called directly, so here we invalidate the control
  // so that our painting logic is used.
  Invalidate();
  return result;
}

void DarkModeComboBox::OnPaint()
{
  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    CRect r;
    GetClientRect(r);
    CPaintDC dc(this);

    // Get details of the control state
    COMBOBOXINFO info = { sizeof(COMBOBOXINFO), 0 };
    GetComboBoxInfo(&info);
    CRect itemR(info.rcItem);
    CRect arrowR(info.rcButton);

    // Get the colours for drawing
    DarkMode::DarkColour border = m_border;
    DarkMode::DarkColour fill = DarkMode::Darkest;
    if (GetDroppedState())
    {
      border = m_activeBorder;
      fill = DarkMode::Dark2;
    }
    else if (dark->CursorInRect(this,r))
    {
      border = m_activeBorder;
      fill = DarkMode::Dark3;
    }

    // Draw the border and background
    dark->DrawBorder(&dc,r,border,fill);

    // Draw the dropdown arrow
    CPen arrowPen;
    arrowPen.CreatePen(PS_SOLID,1,dark->GetColour(border));
    CPen* oldPen = dc.SelectObject(&arrowPen);
    CPoint pt = arrowR.CenterPoint();
    int arrowH = arrowR.Height()/5;
    pt.y += arrowH/2;
    for (int i = 0; i < arrowH; i++)
    {
      dc.MoveTo(pt.x-i,pt.y-i);
      dc.LineTo(pt.x-i+1,pt.y-i);
      dc.MoveTo(pt.x+i,pt.y-i);
      dc.LineTo(pt.x+i+1,pt.y-i);
    }

    // Draw the selected item
    CString itemText;
    int itemIndex = GetCurSel();
    if (itemIndex != CB_ERR)
      GetLBText(itemIndex,itemText);
    dc.SetTextColor(dark->GetColour(DarkMode::Fore));
    dc.SetBkMode(TRANSPARENT);
    itemR.left += 2;
    CFont* oldFont = dc.SelectObject(GetFont());
    dc.DrawText(itemText,itemR,DT_LEFT|DT_VCENTER|DT_HIDEPREFIX|DT_END_ELLIPSIS);

    // Draw the focus rectangle, if needed
    if (!GetDroppedState() && (CWnd::GetFocus() == this))
    {
      if ((SendMessage(WM_QUERYUISTATE) & UISF_HIDEFOCUS) == 0)
      {
        dc.SetTextColor(dark->GetColour(DarkMode::Fore));
        dc.SetBkColor(dark->GetColour(DarkMode::Back));
        itemR.left -= 2;
        dc.DrawFocusRect(itemR);
      }
    }

    dc.SelectObject(oldFont);
    dc.SelectObject(oldPen);
  }
  else
    Default();
}

BEGIN_MESSAGE_MAP(DarkModeGroupBox, CButton)
  ON_WM_PAINT()
END_MESSAGE_MAP()

void DarkModeGroupBox::OnPaint()
{
  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    CRect r;
    GetClientRect(r);
    CPaintDC dc(this);

    CFont* oldFont = dc.SelectObject(GetFont());
    TEXTMETRIC metrics;
    dc.GetTextMetrics(&metrics);

    r.top += metrics.tmHeight/2;
    dark->DrawBorder(&dc,r,DarkMode::Dark1,DarkMode::No_Colour);
    r.top -= metrics.tmHeight/2;

    CString label;
    GetWindowText(label);
    label.Insert(0,' ');
    label.AppendChar(' ');
    r.left += metrics.tmAveCharWidth;
    dc.SetTextColor(dark->GetColour(DarkMode::Fore));
    dc.SetBkColor(dark->GetColour(DarkMode::Back));
    dc.SetBkMode(OPAQUE);
    GetParent()->SendMessage(WM_CTLCOLORSTATIC,(WPARAM)dc.GetSafeHdc(),(LPARAM)GetSafeHwnd());
    dc.DrawText(label,r,DT_LEFT|DT_TOP|DT_HIDEPREFIX|DT_SINGLELINE);

    dc.SelectObject(oldFont);
  }
  else
    Default();
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

struct DarkModeRadioButton::Impl
{
  ImagePNG radio;

  void DrawRadio(ImagePNG& image, int radioIndex, COLORREF colour)
  {
    BYTE r = GetRValue(colour);
    BYTE g = GetGValue(colour);
    BYTE b = GetBValue(colour);

    CSize sz = image.Size();
    for (int y = 0; y < sz.cy; y++)
    {
      for (int x = 0; x < sz.cx; x++)
      {
        BYTE* dest = image.Pixels() + (((y*sz.cx) + x) * sizeof(DWORD));
        BYTE* src = radio.Pixels() + (((y*radio.Size().cx) + x + (radioIndex*sz.cx)) * sizeof(DWORD));

        BYTE a = src[3];
        switch (a)
        {
        case 0x00:
          break;
        case 0xff:
          dest[0] = b;
          dest[1] = g;
          dest[2] = r;
          break;
        default:
          dest[0] = ((b*a)+(dest[0]*(0xff-a)))>>8;
          dest[1] = ((g*a)+(dest[1]*(0xff-a)))>>8;
          dest[2] = ((r*a)+(dest[2]*(0xff-a)))>>8;
          break;
        }
        dest[3] = 0xff;
      }
    }
  }
};

DarkModeRadioButton::DarkModeRadioButton()
{
  m_impl = new Impl;
}

DarkModeRadioButton::~DarkModeRadioButton()
{
  delete m_impl;
}

BOOL DarkModeRadioButton::SubclassDlgItem(UINT id, CWnd* parent, UINT imageId)
{
  if (CWnd::SubclassDlgItem(id,parent))
  {
    ImagePNG img;
    if (img.LoadResource(imageId))
    {
      // Get the the size of the radio button image from the font height
      CDC* dc = GetDC();
      CFont* oldFont = dc->SelectObject(GetFont());
      TEXTMETRIC metrics;
      dc->GetTextMetrics(&metrics);
      dc->SelectObject(oldFont);
      ReleaseDC(dc);
      int imgSize = metrics.tmHeight;

      m_impl->radio.Scale(img,CSize(3*imgSize,imgSize));
      return TRUE;
    }
  }
  return FALSE;
}

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
        // Get the the size of the radio button
        int btnSize = m_impl->radio.Size().cy;
        int btnY = (r.Height()-btnSize)/2;

        // Get the colours for drawing
        DarkMode::DarkColour back = DarkMode::No_Colour;
        DarkMode::DarkColour fore = DarkMode::Dark1;
        if (nmcd->uItemState & CDIS_HOT)
          fore = DarkMode::Fore;
        if (nmcd->uItemState & CDIS_SELECTED)
        {
          fore = DarkMode::Fore;
          back = DarkMode::Dark3;
        }

        // Prepare the image for the radio button
        CRect btnR(r.TopLeft(),CSize(btnSize,btnSize));
        btnR.OffsetRect(0,btnY);
        ImagePNG image;
        image.Create(btnR.Size());
        image.Blend(dark->GetColour(DarkMode::Back));

        // Draw the radio button
        m_impl->DrawRadio(image,1,dark->GetColour(back));
        m_impl->DrawRadio(image,0,dark->GetColour(fore));
        if (GetCheck() == BST_CHECKED)
          m_impl->DrawRadio(image,2,dark->GetColour(fore));

        // Draw the radio button into the window
        image.Draw(dc,btnR.TopLeft());

        // Get the bounding rectangle for the label
        CFont* oldFont = dc->SelectObject(GetFont());
        TEXTMETRIC metrics;
        dc->GetTextMetrics(&metrics);
        int textY = (r.Height()-metrics.tmHeight)/2;
        CRect textR(r);
        textR.OffsetRect((3*btnSize)/2,textY);

        // Draw the label
        CString label;
        GetWindowText(label);
        dc->SetTextColor(dark->GetColour(DarkMode::Fore));
        dc->SetBkMode(TRANSPARENT);
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

        dc->SelectObject(oldFont);
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
              if (dark->CursorInRect(this,r))
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
  ON_WM_CTLCOLOR()
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()

HBRUSH DarkModeToolBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
  HBRUSH brush = CToolBar::OnCtlColor(pDC,pWnd,nCtlColor);
  if (nCtlColor == CTLCOLOR_LISTBOX)
  {
    // For the dropdown list of any combo boxes created as children of this toolbar
    DarkMode* dark = DarkMode::GetActive(this);
    if (dark)
    {
      brush = *(dark->GetBrush(DarkMode::Darkest));
      pDC->SetTextColor(dark->GetColour(DarkMode::Fore));
    }
  }
  return brush;
}

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
