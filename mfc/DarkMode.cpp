#include "stdafx.h"
#include "DarkMode.h"

#include "DpiFunctions.h"
#include "ImagePNG.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static bool CheckWindowsVersion(DWORD major, DWORD minor, DWORD build)
{
  static DWORD os_major = 0, os_minor = 0, os_build = 0;

  if (os_major == 0)
  {
    typedef void(__stdcall *RTLGETNTVERSIONNUMBERS)(LPDWORD, LPDWORD, LPDWORD);
    RTLGETNTVERSIONNUMBERS RtlGetNtVersionNumbers =
      (RTLGETNTVERSIONNUMBERS)GetProcAddress(GetModuleHandle("ntdll.dll"),"RtlGetNtVersionNumbers");
    if (RtlGetNtVersionNumbers)
    {
      (*RtlGetNtVersionNumbers)(&os_major,&os_minor,&os_build);
      os_build &= 0x0FFFFFFF;
    }
  }

  if (major != os_major)
    return (major < os_major);
  if (minor != os_minor)
    return (minor < os_minor);
  return (build <= os_build);
}

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

bool DarkMode::IsEnabled(const char* path)
{
  if (path)
  {
    CRegKey key;
    if (key.Open(HKEY_CURRENT_USER,path,KEY_READ|KEY_WRITE) == ERROR_SUCCESS)
    {
      DWORD disable = 0;
      if (key.QueryDWORDValue("DisableDarkMode",disable) == ERROR_SUCCESS)
      {
        if (disable != 0)
          return false;
      }
      else
      {
        // Ensure key exists so that the user can disable dark mode
        key.SetDWORDValue("DisableDarkMode",0);
      }
    }
  }

  // No dark mode if high contrast is active
  HIGHCONTRAST contrast = { sizeof(contrast), 0 };
  if (::SystemParametersInfo(SPI_GETHIGHCONTRAST,sizeof(contrast),&contrast,FALSE))
  {
    if (contrast.dwFlags & HCF_HIGHCONTRASTON)
      return false;
  }

  CRegKey sysKey;
  LPCSTR sysPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
  if (sysKey.Open(HKEY_CURRENT_USER,sysPath,KEY_READ) == ERROR_SUCCESS)
  {
    DWORD theme = 0;
    if (sysKey.QueryDWORDValue("AppsUseLightTheme",theme) == ERROR_SUCCESS)
    {
      if (theme == 0)
        return true;
    }
  }

  return false;
}

DarkMode* DarkMode::GetEnabled(const char* path)
{
  if (IsEnabled(path))
    return new DarkMode();
  return NULL;
}

DarkMode* DarkMode::GetActive(CWnd* wnd)
{
  CWnd* frame = wnd->IsFrameWnd() ? wnd : wnd->GetParentFrame();
  return (DarkMode*)frame->SendMessage(WM_DARKMODE_ACTIVE);
}

void DarkMode::SetAppDarkMode(void)
{
  if (CheckWindowsVersion(10,0,18362)) // Windows 10 build 1903 "19H1"
  {
    HMODULE uxtheme = ::LoadLibrary("uxtheme.dll");
    if (uxtheme != 0)
    {
      typedef int(__stdcall *SETPREFERREDAPPMODE)(int);
      SETPREFERREDAPPMODE SetPreferredAppMode =
        (SETPREFERREDAPPMODE)::GetProcAddress(uxtheme,MAKEINTRESOURCE(135));
      if (SetPreferredAppMode)
        (*SetPreferredAppMode)(1);
      ::FreeLibrary(uxtheme);
    }
  }
}

void DarkMode::SetDarkTitle(CWnd* wnd, BOOL dark)
{
  HMODULE dwm = ::LoadLibrary("dwmapi.dll");
  if (dwm != 0)
  {
    typedef HRESULT(__stdcall *DWMSETWINDOWATTRIBUTE)(HWND,DWORD,LPCVOID,DWORD);
    DWMSETWINDOWATTRIBUTE DwmSetWindowAttribute =
      (DWMSETWINDOWATTRIBUTE)::GetProcAddress(dwm,"DwmSetWindowAttribute");
    if (DwmSetWindowAttribute)
      (*DwmSetWindowAttribute)(wnd->GetSafeHwnd(),DWMWA_USE_IMMERSIVE_DARK_MODE,&dark,sizeof dark);
    ::FreeLibrary(dwm);
  }
}

void DarkMode::Set(CFrameWnd* frame, DarkMode* dark)
{
  SetDarkTitle(frame,dark != NULL);
  frame->RedrawWindow(NULL,NULL,RDW_ERASE|RDW_INVALIDATE|RDW_ALLCHILDREN);
}

void DarkMode::Set(CDialog* dlg, DarkMode* dark)
{
  SetDarkTitle(dlg,dark != NULL);
  dlg->RedrawWindow(NULL,NULL,RDW_ERASE|RDW_INVALIDATE|RDW_ALLCHILDREN);
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

void DarkMode::DrawRectangle(CDC* dc, const CRect& r, DarkColour border, DarkColour fill)
{
  CPen pen;
  pen.CreatePen(PS_SOLID,1,GetColour(border));

  CBrush* oldBrush = dc->SelectObject(GetBrush(fill));
  CPen* oldPen = dc->SelectObject(&pen);
  dc->Rectangle(r);
  dc->SelectObject(oldPen);
  dc->SelectObject(oldBrush);
}

void DarkMode::DrawControlBorder(CDC* dc, const CRect& r, DarkColour border, DarkColour back, DarkColour fill)
{
  DrawRectangle(dc,r,border,fill);

  // Slightly rounded corners for Windows 11
  if (CheckWindowsVersion(10,0,22000))
  {
    BYTE i1 = GetRValue(GetColour(border));
    BYTE i2 = GetRValue(GetColour(back));
    BYTE i3 = GetRValue(GetColour(fill));

    auto DrawRound=[&](int x, int y, int xs, int ys)
    {
      double f = 0.33;
      BYTE i = (BYTE)((i1*f)+(i2*(1.0-f)));
      dc->SetPixel(x,y,RGB(i,i,i));
      f = 0.18;
      i = (BYTE)((i1*f)+(i3*(1.0-f)));
      dc->SetPixel(x+xs,y+ys,RGB(i,i,i));
    };

    DrawRound(r.left,r.top,1,1);
    DrawRound(r.right-1,r.top,-1,1);
    DrawRound(r.left,r.bottom-1,1,-1);
    DrawRound(r.right-1,r.bottom-1,-1,-1);
  }
}

void DarkMode::DrawButtonBorder(CDC* dc, const CRect& r, DarkColour border, DarkColour back, DarkColour fill)
{
  DrawRectangle(dc,r,border,fill);

  // Rounded corners for Windows 11
  if (CheckWindowsVersion(10,0,22000))
  {
    BYTE i1 = GetRValue(GetColour(border));
    BYTE i2 = GetRValue(GetColour(back));
    BYTE i3 = GetRValue(GetColour(fill));

    auto DrawRound=[&](int x, int y, int xs, int ys)
    {
      dc->SetPixel(x,y,RGB(i2,i2,i2));
      double f = 0.12;
      BYTE i = (BYTE)((i1*f)+(i2*(1.0-f)));
      dc->SetPixel(x+xs,y,RGB(i,i,i));
      dc->SetPixel(x,y+ys,RGB(i,i,i));
      f = 0.76;
      i = (BYTE)((i1*f)+(i2*(1.0-f)));
      dc->SetPixel(x+xs+xs,y,RGB(i,i,i));
      dc->SetPixel(x,y+ys+ys,RGB(i,i,i));
      f = 0.93;
      i = (BYTE)((i1*f)+(i2*(1.0-f)));
      dc->SetPixel(x+xs,y+ys,RGB(i,i,i));
      f = 0.45;
      i = (BYTE)((i1*f)+(i3*(1.0-f)));
      dc->SetPixel(x+xs+xs,y+ys,RGB(i,i,i));
      dc->SetPixel(x+xs,y+ys+ys,RGB(i,i,i));
    };

    DrawRound(r.left,r.top,1,1);
    DrawRound(r.right-1,r.top,-1,1);
    DrawRound(r.left,r.bottom-1,1,-1);
    DrawRound(r.right-1,r.bottom-1,-1,-1);
  }
}

void DarkMode::DrawEditBorder(CDC* dc, const CRect& r, DarkColour border, DarkColour back, DarkColour fill, bool focus)
{
  DrawControlBorder(dc,r,border,back,fill);

  // Emphasis on the bottom border for Windows 11
  if (focus && CheckWindowsVersion(10,0,22000))
  {
    CPen emphasisPen;
    emphasisPen.CreatePen(PS_SOLID,1,GetColour(DarkMode::Fore));
    CPen* oldPen = dc->SelectObject(&emphasisPen);
    dc->MoveTo(r.left,r.bottom-1);
    dc->LineTo(r.right,r.bottom-1);
    dc->MoveTo(r.left,r.bottom-2);
    dc->LineTo(r.right,r.bottom-2);
    dc->SelectObject(oldPen);
  }
}

CRect DarkMode::PrepareNonClientBorder(CWnd* wnd, CWindowDC& dc)
{
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

  // Return the rectangle to draw into
  return rw;
}

bool DarkMode::CursorInRect(CWnd* wnd, CRect r)
{
  CPoint cursor;
  ::GetCursorPos(&cursor);
  wnd->ClientToScreen(r);
  return r.PtInRect(cursor);
}

DarkMode::DarkColour DarkMode::GetBackground(CWnd* wnd)
{
  CWnd* parent = wnd->GetParent();
  if (parent->IsKindOf(RUNTIME_CLASS(DarkModeParentWnd)))
    return ((DarkModeParentWnd*)parent)->GetDarkBackground();
  else if (parent->IsKindOf(RUNTIME_CLASS(DarkModePropertyPage)))
    return DarkMode::Back;
  else if (parent->IsKindOf(RUNTIME_CLASS(CFormView)))
    return DarkMode::Back;
  return DarkMode::Darkest;
}

IMPLEMENT_DYNAMIC(DarkModeParentWnd, CWnd)

// Dark mode controls: DarkModeButton

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

        if (GetStyle() & BS_DEFPUSHBUTTON)
          border = DarkMode::Fore;
        if (nmcd->uItemState & CDIS_SELECTED)
          fill = DarkMode::Dark1;
        else if (nmcd->uItemState & CDIS_HOT)
          fill = DarkMode::Dark2;
        if (nmcd->uItemState & CDIS_DISABLED)
        {
          border = DarkMode::Dark3;
          text = DarkMode::Dark3;
        }
        dark->DrawButtonBorder(dc,r,border,dark->GetBackground(this),fill);

        CString label;
        GetWindowText(label);
        dc->SetTextColor(dark->GetColour(text));
        dc->SetBkMode(TRANSPARENT);
        UINT dtFlags = DT_CENTER|DT_VCENTER|DT_SINGLELINE;
        UINT uiState = (UINT)SendMessage(WM_QUERYUISTATE);
        if (uiState & UISF_HIDEACCEL)
          dtFlags |= DT_HIDEPREFIX;
        CFont* oldFont = dc->SelectObject(GetFont());
        dc->DrawText(label,r,dtFlags);

        if (CWnd::GetFocus() == this)
        {
          if ((uiState & UISF_HIDEFOCUS) == 0)
          {
            r.DeflateRect(2,2);
            dc->SetTextColor(dark->GetColour(DarkMode::Fore));
            dc->SetBkColor(dark->GetColour(DarkMode::Back));
            dc->DrawFocusRect(r);
          }
        }

        dc->SelectObject(oldFont);
      }
      break;
    }
  }
}

// Dark mode controls: DarkModeCheckButton

IMPLEMENT_DYNAMIC(DarkModeCheckButton, CButton)

BEGIN_MESSAGE_MAP(DarkModeCheckButton, CButton)
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()

struct DarkModeCheckButton::Impl
{
  int border = 0;
  ImagePNG check;
};

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
      m_impl->border = (metrics.tmHeight) < 20 ? 2 : 3;
      int imgSize = metrics.tmHeight - (2 * m_impl->border);

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
        // Cleat the client area of the control
        DarkMode::DarkColour back = dark->GetBackground(this);
        dc->FillSolidRect(r,dark->GetColour(back));

        // Get the the size of the check box
        int btnSize = m_impl->check.Size().cy + (2 * m_impl->border);
        int btnY = (r.Height()-btnSize)/2;

        // Get the colours for drawing
        DarkMode::DarkColour fore = DarkMode::Dark1;
        DarkMode::DarkColour fill = back;
        if (nmcd->uItemState & CDIS_HOT)
          fore = DarkMode::Fore;
        if (nmcd->uItemState & CDIS_SELECTED)
        {
          fore = DarkMode::Fore;
          fill = DarkMode::Dark3;
        }
        if (nmcd->uItemState & CDIS_DISABLED)
          fore = DarkMode::Dark3;

        // Draw the border and background of the button
        CRect btnR(r.TopLeft(),CSize(btnSize,btnSize));
        btnR.OffsetRect(0,btnY);
        dark->DrawControlBorder(dc,btnR,fore,dark->GetBackground(this),fill);

        // Draw the check, if needed
        if (GetCheck() == BST_CHECKED)
        {
          ImagePNG image;
          image.Copy(m_impl->check);
          image.Fill(dark->GetColour(fore));
          image.Blend(dark->GetColour(fill));
          image.Draw(dc,btnR.TopLeft()+CPoint(m_impl->border,m_impl->border));
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
        UINT dtFlags = DT_LEFT|DT_TOP;
        UINT uiState = (UINT)SendMessage(WM_QUERYUISTATE);
        if (uiState & UISF_HIDEACCEL)
          dtFlags |= DT_HIDEPREFIX;
        dc->DrawText(label,textR,dtFlags);

        // Draw the focus rectangle, if needed
        if (HasFocusRect(uiState))
        {
          dc->DrawText(label,textR,dtFlags|DT_CALCRECT);
          textR.InflateRect(2,0);
          dc->SetTextColor(dark->GetColour(DarkMode::Fore));
          dc->SetBkColor(dark->GetColour(DarkMode::Back));
          dc->DrawFocusRect(textR);
        }

        dc->SelectObject(oldFont);
      }
      break;
    }
  }
}

bool DarkModeCheckButton::HasFocusRect(UINT uiState)
{
  return (CWnd::GetFocus() == this) && ((uiState & UISF_HIDEFOCUS) == 0);
}

// Dark mode controls: DarkModeComboBox

BEGIN_MESSAGE_MAP(DarkModeComboBox, CComboBox)
  ON_WM_PAINT()
  ON_WM_CTLCOLOR()
  ON_MESSAGE(CB_SETCURSEL, OnSetCurSel)
END_MESSAGE_MAP()

void DarkModeComboBox::SetDarkMode(DarkMode* dark)
{
  if (GetSafeHwnd() != 0)
  {
    // Set a theme on the list control so that, if there is a scrollbar on the list, it is dark
    COMBOBOXINFO info = { sizeof (COMBOBOXINFO), 0 };
    GetComboBoxInfo(&info);
    if (info.hwndList != 0)
    {
      LPCWSTR theme = dark ? L"DarkMode_Explorer" : NULL;
      ::SetWindowTheme(info.hwndList,theme,NULL);
    }
  }
}

void DarkModeComboBox::SetDarkBorder(DarkMode::DarkColour colour, DarkMode::DarkColour activeColour)
{
  m_border = colour;
  m_activeBorder = activeColour;
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
    bool disabled = ((GetStyle() & WS_DISABLED) != 0);

    // Get the colours for drawing
    DarkMode::DarkColour border = m_border;
    DarkMode::DarkColour fill = DarkMode::Darkest;
    if (disabled)
      border = DarkMode::Dark3;
    else if (GetDroppedState())
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
    dark->DrawControlBorder(&dc,r,border,dark->GetBackground(this),fill);

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

    if (GetStyle() & CBS_DROPDOWNLIST)
    {
      // Draw the selected item
      CString itemText;
      int itemIndex = GetCurSel();
      if (itemIndex != CB_ERR)
        GetLBText(itemIndex,itemText);
      dc.SetTextColor(dark->GetColour(disabled ? DarkMode::Dark3 : DarkMode::Fore));
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
    }
    dc.SelectObject(oldPen);
  }
  else
    Default();
}

HBRUSH DarkModeComboBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
  HBRUSH brush = CComboBox::OnCtlColor(pDC,pWnd,nCtlColor);
  if (nCtlColor == CTLCOLOR_EDIT)
  {
    // For the edit control of an editable combo box
    DarkMode* dark = DarkMode::GetActive(this);
    if (dark)
    {
      DarkMode::DarkColour fill = DarkMode::Darkest;
      CRect r;
      GetClientRect(r);
      if (dark->CursorInRect(this,r))
        fill = DarkMode::Dark3;

      brush = *(dark->GetBrush(fill));
      pDC->SetBkColor(dark->GetColour(fill));
      pDC->SetTextColor(dark->GetColour(DarkMode::Fore));
    }
  }
  return brush;
}

LRESULT DarkModeComboBox::OnSetCurSel(WPARAM, LPARAM)
{
  LRESULT result = Default();

  // Changing the combo box selection will cause the internal painting logic
  // of the control to be called directly, so here we force a redraw.
  RedrawWindow(NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
  return result;
}

// Dark mode controls: DarkModeEdit

IMPLEMENT_DYNAMIC(DarkModeEdit, CEdit)

BEGIN_MESSAGE_MAP(DarkModeEdit, CEdit)
  ON_WM_NCPAINT()
END_MESSAGE_MAP()

void DarkModeEdit::OnNcPaint()
{
  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    CWindowDC dc(this);
    bool focus = (CWnd::GetFocus() == this);
    DarkMode::DarkColour border = focus ? DarkMode::Dark1 : DarkMode::Dark2;
    CRect r = dark->PrepareNonClientBorder(this,dc);
    dark->DrawEditBorder(&dc,r,border,dark->GetBackground(this),DarkMode::Darkest,focus);
    dc.SelectClipRgn(NULL);
  }
  else
    Default();
}

// Dark mode controls: DarkModeGroupBox

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
    dark->DrawRectangle(&dc,r,DarkMode::Dark2,DarkMode::No_Colour);
    r.top -= metrics.tmHeight/2;

    CString label;
    GetWindowText(label);
    label.Insert(0,' ');
    label.AppendChar(' ');
    r.left += metrics.tmAveCharWidth;
    dc.SetTextColor(dark->GetColour(DarkMode::Fore));
    dc.SetBkColor(dark->GetColour(DarkMode::Back));
    GetParent()->SendMessage(WM_CTLCOLORSTATIC,(WPARAM)dc.GetSafeHdc(),(LPARAM)GetSafeHwnd());
    dc.SetBkMode(OPAQUE);
    dc.DrawText(label,r,DT_LEFT|DT_TOP|DT_HIDEPREFIX|DT_SINGLELINE);

    dc.SelectObject(oldFont);
  }
  else
    Default();
}

// Dark mode controls: DarkModeListBox

BEGIN_MESSAGE_MAP(DarkModeListBox, CListBox)
  ON_WM_NCPAINT()
END_MESSAGE_MAP()

void DarkModeListBox::SetDarkMode(DarkMode* dark)
{
  if (GetSafeHwnd() != 0)
  {
    LPCWSTR theme = dark ? L"DarkMode_Explorer" : NULL;
    ::SetWindowTheme(GetSafeHwnd(),theme,NULL);
  }
}

void DarkModeListBox::OnNcPaint()
{
  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    // Do default drawing first, to draw any scrollbar
    Default();

    CWindowDC dc(this);

    // Get the window and client rectangles, in the window co-ordinate space
    CRect rw, rc;
    GetWindowRect(rw);
    ScreenToClient(rw);
    GetClientRect(rc);
    rc.OffsetRect(-rw.TopLeft());
    rw.OffsetRect(-rw.TopLeft());

    // There may be a vertical scrollbar, so adjust the client area
    // (which is excluded from drawing) to include it
    rc.right = rw.right - (rc.left - rw.left);

    // Draw the border around the window
    dc.ExcludeClipRect(rc);
    dc.IntersectClipRect(rw);
    DarkMode::DarkColour border =
      (CWnd::GetFocus() == this) ? DarkMode::Dark1 : DarkMode::Dark2;
    dark->DrawControlBorder(&dc,rw,border,dark->GetBackground(this),DarkMode::Darkest);
    dc.SelectClipRgn(NULL);
  }
  else
    Default();
}

// Dark mode controls: DarkModeProgressCtrl

BEGIN_MESSAGE_MAP(DarkModeProgressCtrl, CProgressCtrl)
  ON_WM_NCPAINT()
END_MESSAGE_MAP()

void DarkModeProgressCtrl::SetDarkMode(DarkMode* dark)
{
  if (GetSafeHwnd() != 0)
  {
    LPCWSTR theme = dark ? L"" : NULL;
    ::SetWindowTheme(GetSafeHwnd(),theme,theme);

    if (dark)
    {
      SetBkColor(dark->GetColour(DarkMode::Back));
      SetBarColor(dark->GetColour(DarkMode::Dark1));
    }
  }
}

void DarkModeProgressCtrl::OnNcPaint()
{
  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    CWindowDC dc(this);
    CRect r = dark->PrepareNonClientBorder(this,dc);
    dark->DrawRectangle(&dc,r,DarkMode::Dark3,DarkMode::Darkest);
    dc.SelectClipRgn(NULL);
  }
  else
    Default();
}

// Dark mode controls: DarkModePropertyPage

IMPLEMENT_DYNAMIC(DarkModePropertyPage, CPropertyPage)

BEGIN_MESSAGE_MAP(DarkModePropertyPage, CPropertyPage)
  ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

DarkModePropertyPage::DarkModePropertyPage(UINT id) : CPropertyPage(id)
{
}

void DarkModePropertyPage::SetDarkMode(DarkMode* dark, bool init)
{
}

HBRUSH DarkModePropertyPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
  HBRUSH brush = CPropertyPage::OnCtlColor(pDC,pWnd,nCtlColor);
  DarkMode* dark = DarkMode::GetActive(this);

  switch (nCtlColor)
  {
  case CTLCOLOR_DLG:
  case CTLCOLOR_STATIC:
  case CTLCOLOR_EDIT:
    if (dark)
    {
      brush = *(dark->GetBrush(DarkMode::Back));
      pDC->SetBkColor(dark->GetColour(DarkMode::Back));
      pDC->SetTextColor(dark->GetColour(DarkMode::Fore));
    }
    break;
  case CTLCOLOR_LISTBOX: // For combo box dropdown lists
    if (dark)
    {
      brush = *(dark->GetBrush(DarkMode::Darkest));
      pDC->SetTextColor(dark->GetColour(DarkMode::Fore));
    }
    break;
  }
  return brush;
}

// Dark mode controls: DarkModePropertySheet

IMPLEMENT_DYNAMIC(DarkModePropertySheet, CPropertySheet)

BEGIN_MESSAGE_MAP(DarkModePropertySheet, CPropertySheet)
  ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

DarkModePropertySheet::DarkModePropertySheet(UINT caption, CWnd* parent) : CPropertySheet(caption,parent)
{
}

DarkModePropertySheet::DarkModePropertySheet(LPCSTR caption) : CPropertySheet(caption)
{
}

BOOL DarkModePropertySheet::OnInitDialog()
{
  CPropertySheet::OnInitDialog();

  m_okBtn.SubclassDlgItem(IDOK,this);
  m_cancelBtn.SubclassDlgItem(IDCANCEL,this);
  m_tabCtrl.SubclassWindow((HWND)SendMessage(PSM_GETTABCONTROL));
  return TRUE;
}

BOOL DarkModePropertySheet::OnEraseBkgnd(CDC* dc)
{
  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    CRect r;
    GetClientRect(r);
    dc->FillSolidRect(r,dark->GetColour(DarkMode::Darkest));
    return TRUE;
  }
  else
    return CPropertySheet::OnEraseBkgnd(dc);
}

void DarkModePropertySheet::SetDarkMode(DarkMode* dark, bool init)
{
  if (GetSafeHwnd() != 0)
  {
    DarkMode::SetDarkTitle(this,dark != NULL);

    for (int i = 0; i < GetPageCount(); i++)
    {
      CPropertyPage* page = GetPage(i);
      if (page->IsKindOf(RUNTIME_CLASS(DarkModePropertyPage)))
        ((DarkModePropertyPage*)page)->SetDarkMode(dark,init);
    }

    RedrawWindow(NULL,NULL,RDW_ERASE|RDW_INVALIDATE|RDW_ALLCHILDREN);
  }
}

// Dark mode controls: DarkModeRadioButton

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
        DarkMode::DarkColour back = dark->GetBackground(this);
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
        UINT dtFlags = DT_LEFT|DT_TOP;
        UINT uiState = (UINT)SendMessage(WM_QUERYUISTATE);
        if (uiState & UISF_HIDEACCEL)
          dtFlags |= DT_HIDEPREFIX;
        dc->DrawText(label,textR,dtFlags);

        // Draw the focus rectangle, if needed
        if (CWnd::GetFocus() == this)
        {
          if ((uiState & UISF_HIDEFOCUS) == 0)
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

// Dark mode controls: DarkModeRichEditCtrl

IMPLEMENT_DYNAMIC(DarkModeRichEditCtrl, CRichEditCtrl)

BEGIN_MESSAGE_MAP(DarkModeRichEditCtrl, CRichEditCtrl)
  ON_WM_NCPAINT()
END_MESSAGE_MAP()

void DarkModeRichEditCtrl::SetDarkMode(DarkMode* dark, DarkMode::DarkColour darkBackground)
{
  LPCWSTR theme = dark ? L"DarkMode_Explorer" : NULL;
  ::SetWindowTheme(GetSafeHwnd(),theme,NULL);

  CHARFORMAT format = { 0 };
  format.cbSize = sizeof format;
  format.dwMask = CFM_COLOR;
  if (dark)
  {
    format.crTextColor = dark->GetColour(DarkMode::Fore);
    SetBackgroundColor(FALSE,dark->GetColour(darkBackground));
  }
  else
  {
    format.crTextColor = ::GetSysColor(COLOR_BTNTEXT);
    SetBackgroundColor(FALSE,GetSysColor(COLOR_3DFACE));
  }
  SetDefaultCharFormat(format);
}

void DarkModeRichEditCtrl::OnNcPaint()
{
  Default();

  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    CWindowDC dc(this);

    // Get the window and client rectangles, in the window co-ordinate space
    CRect rw, rc;
    GetWindowRect(rw);
    ScreenToClient(rw);
    GetClientRect(rc);
    rc.OffsetRect(-rw.TopLeft());
    rw.OffsetRect(-rw.TopLeft());

    // Account for the possibility of a scrollbar by making the right and bottom
    // borders the same size as the top and left borders.
    rc.right = rw.right - (rc.left - rw.left);
    rc.bottom = rw.bottom - (rc.top - rw.top);

    // Make the clipping region the window rectangle, excluding the client rectangle
    dc.ExcludeClipRect(rc);
    dc.IntersectClipRect(rw);

    dark->DrawRectangle(&dc,rw,DarkMode::Dark3,DarkMode::Back);
    dc.SelectClipRgn(NULL);
  }
}

// Dark mode controls: DarkModeSliderCtrl

BEGIN_MESSAGE_MAP(DarkModeSliderCtrl, CSliderCtrl)
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
  ON_WM_ERASEBKGND()
  ON_MESSAGE(TBM_SETPOS, OnSetPos)
END_MESSAGE_MAP()

void DarkModeSliderCtrl::OnCustomDraw(NMHDR* nmhdr, LRESULT* result)
{
  NMCUSTOMDRAW* nmcd = (NMCUSTOMDRAW*)nmhdr;
  *result = CDRF_DODEFAULT;

  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    CDC* dc = CDC::FromHandle(nmcd->hdc);

    switch (nmcd->dwDrawStage)
    {
    case CDDS_PREERASE:
    case CDDS_PREPAINT:
      *result = CDRF_SKIPDEFAULT;
      {
        // Draw the slider channel
        CRect cr;
        GetChannelRect(cr);
        dark->DrawRectangle(dc,cr,DarkMode::Dark3,DarkMode::Darkest);

        // Draw the slider thumb
        CRect tr;
        GetThumbRect(tr);
        DarkMode::DarkColour thumb = DarkMode::Dark2;
        if (nmcd->uItemState & CDIS_SELECTED)
          thumb = DarkMode::Fore;
        else
        {
          // Is the mouse over the thumb?
          if (dark->CursorInRect(this,tr))
            thumb = DarkMode::Dark1;
        }
        dc->FillSolidRect(tr,dark->GetColour(thumb));

        // Draw slider ticks
        if ((GetStyle() & TBS_NOTICKS) == 0)
        {
          CPen tickPen;
          tickPen.CreatePen(PS_SOLID,2,dark->GetColour(DarkMode::Dark2));
          CPen* oldPen = dc->SelectObject(&tickPen);
          int y = tr.bottom+1;
          int x = cr.left + (tr.Width()/2);
          dc->MoveTo(x,y);
          dc->LineTo(x,y+8);
          for (UINT i = 0; i < GetNumTics(); i++)
          {
            x = GetTicPos(i);
            dc->MoveTo(x,y);
            dc->LineTo(x,y+8);
          }
          x = cr.right - (tr.Width()/2);
          dc->MoveTo(x,y);
          dc->LineTo(x,y+8);
          dc->SelectObject(oldPen);
        }

        // Draw a focus rectangle
        if (CWnd::GetFocus() == this)
        {
          UINT uiState = (UINT)SendMessage(WM_QUERYUISTATE);
          if ((uiState & UISF_HIDEFOCUS) == 0)
          {
            CRect r;
            GetClientRect(r);
            dc->SetTextColor(dark->GetColour(DarkMode::Fore));
            dc->SetBkColor(dark->GetColour(DarkMode::Back));
            dc->DrawFocusRect(r);
          }
        }
      }
      break;
    }
  }
}

BOOL DarkModeSliderCtrl::OnEraseBkgnd(CDC* pDC)
{
  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
    return TRUE;
  else
    return CSliderCtrl::OnEraseBkgnd(pDC);
}

LRESULT DarkModeSliderCtrl::OnSetPos(WPARAM, LPARAM)
{
  LRESULT result = Default();

  // Changing the slider position will cause the internal painting logic
  // of the control to be called directly, so here we force a redraw.
  RedrawWindow(NULL,NULL,RDW_INVALIDATE);
  return result;
}

// Dark mode controls: DarkModeStatic

BEGIN_MESSAGE_MAP(DarkModeStatic, CStatic)
  ON_WM_PAINT()
  ON_WM_ENABLE()
END_MESSAGE_MAP()

void DarkModeStatic::OnPaint()
{
  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    CRect r;
    GetClientRect(r);
    CPaintDC dc(this);

    CFont* oldFont = dc.SelectObject(GetFont());

    CString label;
    GetWindowText(label);
    DWORD style = GetStyle();
    UINT dtFlags = DT_TOP|DT_HIDEPREFIX;
    if (style & SS_RIGHT)
      dtFlags |= DT_RIGHT;
    GetParent()->SendMessage(WM_CTLCOLORSTATIC,(WPARAM)dc.GetSafeHdc(),(LPARAM)GetSafeHwnd());
    dc.SetBkMode(OPAQUE);
    if (style & WS_DISABLED)
      dc.SetTextColor(dark->GetColour(DarkMode::Dark3));
    dc.DrawText(label,r,dtFlags);

    dc.SelectObject(oldFont);
  }
  else
    Default();
}

void DarkModeStatic::OnEnable(BOOL bEnable)
{
  CStatic::OnEnable(bEnable);

  // Changing the enabled state will cause the internal painting logic
  // of the control to be called directly, so here we force a redraw.
  RedrawWindow(NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
}

// Dark mode controls: DarkModeStatusBar

BEGIN_MESSAGE_MAP(DarkModeStatusBar, CStatusBar)
  ON_WM_PAINT()
END_MESSAGE_MAP()

void DarkModeStatusBar::OnPaint()
{
  UpdateAllPanes(FALSE,TRUE);

  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    CRect r;
    GetClientRect(r);
    CPaintDC dc(this);
    dc.FillSolidRect(r,dark->GetColour(DarkMode::Darkest));

    CPen pen;
    pen.CreatePen(PS_SOLID,1,dark->GetColour(DarkMode::Dark2));
    CPen* oldPen = dc.SelectObject(&pen);

    dc.SetTextColor(dark->GetColour(DarkMode::Fore));
    dc.SetBkMode(TRANSPARENT);
    CFont* oldFont = dc.SelectObject(GetFont());

    for (int i = 0; i < GetCount(); i++)
    {
      CRect ir;
      GetItemRect(i,ir);
      ir.left += m_cxDefaultGap;

      UINT style = GetPaneStyle(i);
      if (!(style & SBPS_STRETCH))
      {
        dc.MoveTo(ir.right-1,ir.top);
        dc.LineTo(ir.right-1,ir.bottom-1);
        ir.right -= m_cxDefaultGap;
      }
      if (!(style & SBPS_DISABLED))
        dc.DrawText(GetPaneText(i),ir,DT_SINGLELINE);
    }

    if (::IsAppThemed())
    {
      HTHEME theme = ::OpenThemeData(GetSafeHwnd(),L"Status");
      if (theme)
      {
        CRect gripRect(r);
        int dpi = DPI::getWindowDPI(this);
        gripRect.left = gripRect.right - DPI::getSystemMetrics(SM_CXHSCROLL,dpi);
        gripRect.top = gripRect.bottom - DPI::getSystemMetrics(SM_CYVSCROLL,dpi);
        ::DrawThemeBackground(theme,dc.GetSafeHdc(),SP_GRIPPER,0,gripRect,NULL);
        ::CloseThemeData(theme);
      }
    }

    dc.SelectObject(oldFont);
    dc.SelectObject(oldPen);
  }
  else
    Default();
}

// Dark mode controls: DarkModeTabCtrl

BEGIN_MESSAGE_MAP(DarkModeTabCtrl, CTabCtrl)
  ON_WM_PAINT()
  ON_WM_MOUSEMOVE()
  ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

void DarkModeTabCtrl::OnPaint()
{
  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    CRect r;
    GetClientRect(r);
    CPaintDC dc(this);

    CRect ir;
    GetItemRect(0,ir);
    int y = ir.bottom;

    dc.FillSolidRect(r,dark->GetColour(DarkMode::Darkest));
    r.top = y;
    dark->DrawRectangle(&dc,r,DarkMode::Dark2,DarkMode::Back);

    int sel = GetCurSel();
    for (int i = 0; i < GetItemCount(); i++)
    {
      GetItemRect(i,ir);
      if (i == 0)
        ir.left = 0;
      ir.bottom = y+1;

      TCITEM item = { TCIF_TEXT|TCIF_STATE,0 };
      char label[256];
      item.pszText = label;
      item.cchTextMax = sizeof(label);
      GetItem(i,&item);

      if (i == sel)
        dc.FillSolidRect(ir,dark->GetColour(DarkMode::Back));
      else if (i == m_mouseOverItem)
        dc.FillSolidRect(ir,dark->GetColour(DarkMode::Dark2));

      dc.SetTextColor(dark->GetColour(DarkMode::Fore));
      dc.SetBkMode(TRANSPARENT);
      CFont* oldFont = dc.SelectObject(GetFont());
      dc.DrawText(label,ir,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
      dc.SelectObject(oldFont);

      if (i == sel)
      {
        CPen pen;
        pen.CreatePen(PS_SOLID,1,dark->GetColour(DarkMode::Dark2));
        CPen* oldPen = dc.SelectObject(&pen);
        dc.MoveTo(ir.left,ir.bottom-1);
        dc.LineTo(ir.left,ir.top);
        dc.LineTo(ir.right,ir.top);
        dc.LineTo(ir.right,ir.bottom);
        dc.SelectObject(oldPen);
      }
    }
  }
  else
    Default();
}

void DarkModeTabCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
  int hotTab = -1;
  for (int i = 0; i < GetItemCount(); i++)
  {
    CRect r;
    GetItemRect(i,r);
    if (r.PtInRect(point))
      hotTab = i;
  }

  if (m_mouseOverItem != hotTab)
  {
    m_mouseOverItem = hotTab;
    Invalidate();

    if (!m_mouseTrack)
    {
      // Listen for the mouse leaving this control
      TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), 0 };
      tme.dwFlags = TME_LEAVE;
      tme.hwndTrack = GetSafeHwnd();
      ::TrackMouseEvent(&tme);
      m_mouseTrack = true;
    }
  }
  CTabCtrl::OnMouseMove(nFlags,point);
}

LRESULT DarkModeTabCtrl::OnMouseLeave(WPARAM, LPARAM)
{
  if (m_mouseOverItem >= 0)
  {
    m_mouseOverItem = -1;
    m_mouseTrack = false;
    Invalidate();
  }
  return Default();
}

// Dark mode controls: DarkModeToolBar

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
