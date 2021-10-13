#include "stdafx.h"
#include "MenuBar.h"
#include "DpiFunctions.h"
#pragma warning(disable : 4996) // Ignore deprecation warning

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace {
MenuBar* menuBar = NULL;
HHOOK msgHook = NULL;
}

IMPLEMENT_DYNAMIC(MenuBar, CToolBar)

BEGIN_MESSAGE_MAP(MenuBar, CToolBar)
  ON_WM_MOUSEMOVE()
  ON_WM_SETTINGCHANGE()
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
  ON_NOTIFY_REFLECT(TBN_DROPDOWN, OnDropDown)
  ON_NOTIFY_REFLECT(TBN_HOTITEMCHANGE, OnHotItemChange)
  ON_UPDATE_COMMAND_UI_RANGE(0, 256, OnUpdateButton)
  ON_MESSAGE(WM_MENUBAR_POPUP, OnMenuPopup)
END_MESSAGE_MAP()

MenuBar::MenuBar()
{
  m_tracking = TRACK_NONE;
  m_popupTrack = -1;
  m_popupNew = -1;
  m_popupMenu = 0;
  m_arrowLeft = false;
  m_arrowRight = false;
  m_escapePressed = false;

  m_useF10 = true;
  m_filterAltX = NULL;
}

MenuBar::~MenuBar()
{
  DeleteBitmaps();
}

void MenuBar::SetUseF10(bool use)
{
  m_useF10 = use;
}

void MenuBar::SetFilterAltX(FilterAltX filter)
{
  m_filterAltX = filter;
}

void MenuBar::AddNoIconId(UINT id)
{
  m_noIconIds.Add(id);
}

BOOL MenuBar::Create(UINT id, CMenu* menu, CWnd* parent)
{
  // Create the menu toolbar
  if (CreateEx(parent,TBSTYLE_FLAT|TBSTYLE_LIST|TBSTYLE_TRANSPARENT,
    WS_CHILD|WS_VISIBLE|CBRS_ALIGN_TOP|CBRS_TOOLTIPS|CBRS_FLYBY) == FALSE)
  {
    return FALSE;
  }
  GetToolBarCtrl().SetBitmapSize(CSize(0,0));
  UpdateFont(DPI::getWindowDPI(parent));

  // Load the menus
  if (id == -1)
    m_menu.Attach(menu->Detach());
  else
    m_menu.LoadMenu(id);
  Update();

  // Remove the existing menu from the frame window
  ::SetMenu(GetParentFrame()->GetSafeHwnd(),0);
  return TRUE;
}

void MenuBar::LoadBitmaps(CBitmap& bitmap, CToolBarCtrl& bar, CSize size, bool alpha)
{
  // Create device contexts compatible with the display
  CDC dcFrom, dcTo;
  dcFrom.CreateCompatibleDC(NULL);
  dcTo.CreateCompatibleDC(NULL);

  // Load the toolbar bitmap into a device context
  CBitmap* oldFromBitmap = dcFrom.SelectObject(&bitmap);

  // Iterate over the toolbar buttons, extracting the bitmap image for each
  for (int i = 0; i < bar.GetButtonCount(); i++)
  {
    TBBUTTON button;
    bar.GetButton(i,&button);

    // Check that the button has a bitmap and is not a separator
    if ((button.iBitmap >= 0) && ((button.fsStyle & TBSTYLE_SEP) == 0))
    {
      // Skip excluded menu items
      bool skip = false;
      for (int iconId = 0; iconId < m_noIconIds.GetSize(); iconId++)
      {
        if (button.idCommand == m_noIconIds[iconId])
          skip = true;
      }
      if (skip)
        continue;

      // Create a new button bitmap
      BITMAPINFO bi = { sizeof(BITMAPINFOHEADER),0 };
      bi.bmiHeader.biPlanes = 1;
      bi.bmiHeader.biBitCount = 32;
      bi.bmiHeader.biCompression = BI_RGB;
      bi.bmiHeader.biWidth = size.cx;
      bi.bmiHeader.biHeight = size.cy;
      DWORD* bits;
      CBitmap buttonBitmap;
      buttonBitmap.Attach(
        ::CreateDIBSection(dcFrom,&bi,DIB_RGB_COLORS,(VOID**)&bits,NULL,0));

      // Copy the image into the button bitmap
      CBitmap* oldToBitmap = dcTo.SelectObject(&buttonBitmap);
      dcTo.BitBlt(0,0,size.cx,size.cy,&dcFrom,size.cx*button.iBitmap,0,SRCCOPY);
      dcTo.SelectObject(oldToBitmap);

      // Set the button bitmap to use pre-computed alpha: that is,
      // the RGB components are already scaled by the alpha channel.
      ::GdiFlush();
      DWORD clear = bits[0];
      for (int y = 0; y < size.cy; y++)
      {
        for (int x = 0; x < size.cx; x++)
        {
          int idx = x+(y*size.cx);
          if (alpha)
          {
            DWORD a = (bits[idx]&0xff000000)>>24;
            if (a == 0)
              bits[idx] = 0;
            else if (a != 0xff)
            {
              DWORD r = (bits[idx]&0x00ff0000)>>16;
              DWORD g = (bits[idx]&0x0000ff00)>>8;
              DWORD b = bits[idx]&0x000000ff;
              r = (r*a)>>8; g = (g*a)>>8; b = (b*a)>>8;
              bits[idx] = (a<<24)|(r<<16)|(g<<8)|b;
            }
          }
          else
          {
            if (bits[idx] == clear)
              bits[idx] = 0;
            else
              bits[idx] = (0xff<<24)|(bits[idx]&0x00ffffff);
          }
        }
      }

      // Make a copy of the bitmap bits
      DWORD* bitsCopy = new DWORD[size.cx*size.cy];
      memcpy(bitsCopy,bits,size.cx*size.cy*sizeof(DWORD));

      Bitmap lookup;
      if (m_bitmaps.Lookup(button.idCommand,lookup) == FALSE)
      {
        Bitmap info((HBITMAP)buttonBitmap.Detach(),size,bits,bitsCopy);
        m_bitmaps.SetAt(button.idCommand,info);
      }
    }
  }
  dcFrom.SelectObject(oldFromBitmap);
}

void MenuBar::LoadBitmaps(ImagePNG& images, CToolBarCtrl& bar, CSize size)
{
  CBitmap menuBitmap;
  menuBitmap.Attach(images.CopyBitmap(&bar));
  if (menuBitmap.GetSafeHandle())
    LoadBitmaps(menuBitmap,bar,size,true);
}

void MenuBar::DeleteBitmaps(void)
{
  UINT id;
  Bitmap bitmap;
  POSITION pos = m_bitmaps.GetStartPosition();
  while (pos != NULL)
  {
    m_bitmaps.GetNextAssoc(pos,id,bitmap);
    ::DeleteObject(bitmap.bitmap);
    delete[] bitmap.initialBits;
  }
  m_bitmaps.RemoveAll();
}

void MenuBar::Update(void)
{
  SetBitmaps(&m_menu);

  // Add menu headings
  if (m_menu.GetMenuItemCount() > 0)
  {
    SetButtons(0,m_menu.GetMenuItemCount());
    for (UINT i = 0; i < (UINT)m_menu.GetMenuItemCount(); i++)
    {
      CString title;
      m_menu.GetMenuString(i,title,MF_BYPOSITION);
      SetButtonInfo(i,i,BTNS_BUTTON|BTNS_AUTOSIZE|BTNS_DROPDOWN,0);
      SetButtonText(i,title);
    }
  }
}

void MenuBar::UpdateFont(int dpi)
{
  if (m_font.GetSafeHandle() != 0)
    m_font.DeleteObject();
  if (DPI::createSystemMenuFont(&m_font,dpi))
    SetFont(&m_font);

  int barHeight = DPI::getSystemMetrics(SM_CYMENU,dpi);

  if (::IsAppThemed())
  {
    HTHEME theme = ::OpenThemeData(GetSafeHwnd(),L"Menu");
    if (theme)
    {
      CDC* dc = GetDC();
      CFont* oldFont = dc->SelectObject(GetFont());

      MARGINS margins = { 0 };
      if (SUCCEEDED(::GetThemeMargins(theme,dc->GetSafeHdc(),
        MENU_BARITEM,MBI_NORMAL,TMT_CONTENTMARGINS,NULL,&margins)))
      {
        TEXTMETRIC metrics;
        dc->GetTextMetrics(&metrics);
        if (barHeight < metrics.tmHeight + margins.cyTopHeight + margins.cyBottomHeight)
          barHeight = metrics.tmHeight + margins.cyTopHeight + margins.cyBottomHeight;
      }

      dc->SelectObject(oldFont);
      ReleaseDC(dc);
      ::CloseThemeData(theme);
    }
  }

  GetToolBarCtrl().SetButtonSize(CSize(0,barHeight));
}

CMenu* MenuBar::GetMenu(void) const
{
  return const_cast<CMenu*>(&m_menu);
}

BOOL MenuBar::TranslateFrameMessage(MSG* msg)
{
  // Stop tracking if the user clicked outside of the menu bar
  if ((msg->message >= WM_LBUTTONDOWN) && (msg->message <= WM_MOUSELAST))
  {
    if ((msg->hwnd != GetSafeHwnd()) && (m_tracking != TRACK_NONE))
      SetTrackingState(TRACK_NONE);
    return FALSE;
  }

  bool alt = ((::GetKeyState(VK_LMENU) & 0x8000) != 0);
  bool shift = ((::GetKeyState(VK_SHIFT) & 0x8000) != 0);
  bool ctrl = ((::GetKeyState(VK_CONTROL) & 0x8000) != 0);

  switch (msg->message)
  {
  case WM_SYSKEYUP:

    // Check for menu key (Alt) without Ctrl/Shift or F10 without Alt/Ctrl/Shift
    if (((msg->wParam == VK_MENU) && !(shift||ctrl)) ||
        (m_useF10 && (msg->wParam == VK_F10) && !(shift||ctrl||alt)))
    {
      switch (m_tracking)
      {
      case TRACK_NONE:
        SetTrackingState(TRACK_BUTTON,0);
        break;
      case TRACK_BUTTON:
        SetTrackingState(TRACK_NONE);
        break;
      }
      return TRUE;
    }
    break;

  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:

    // Check for menu key (Alt) or F10 without Alt/Ctrl/Shift
    if ((msg->wParam == VK_MENU) ||
        (m_useF10 && (msg->wParam == VK_F10) && !(shift||ctrl||alt)))
      return FALSE;

    if (m_tracking == TRACK_BUTTON)
    {
      switch (msg->wParam)
      {
      case VK_LEFT:
        GetToolBarCtrl().SetHotItem(GetNextButton(GetToolBarCtrl().GetHotItem(),true));
        return TRUE;
      case VK_RIGHT:
        GetToolBarCtrl().SetHotItem(GetNextButton(GetToolBarCtrl().GetHotItem(),false));
        return TRUE;
      case VK_UP:
      case VK_DOWN:
      case VK_RETURN:
        PostMessage(WM_MENUBAR_POPUP,GetToolBarCtrl().GetHotItem(),TRUE);
        return TRUE;
      case VK_ESCAPE:
        SetTrackingState(TRACK_NONE);
        return TRUE;
      case VK_SPACE:
        SetTrackingState(TRACK_NONE);
        GetParentFrame()->PostMessage(WM_SYSCOMMAND,SC_KEYMENU,' ');
        return TRUE;
      }
    }

    if ((alt || (m_tracking == TRACK_BUTTON)) && ((msg->wParam >= '0') && (msg->wParam <= 'Z')))
    {
      // Alt-X, or else X while tracking menu buttons
      UINT id;
      if (GetToolBarCtrl().MapAccelerator((CHAR)msg->wParam,&id))
      {
        if (!alt || AllowAltX(msg->wParam))
        {
          PostMessage(WM_MENUBAR_POPUP,id,TRUE);
          return TRUE;
        }
      }
      else if ((m_tracking == TRACK_BUTTON) && !alt)
        return TRUE;
    }

    if (m_tracking != TRACK_NONE)
      SetTrackingState(TRACK_NONE);
    break;
  }
  return FALSE;
}

void MenuBar::OnMouseMove(UINT nFlags, CPoint pt)
{
  // Let the mouse change the active button in button tracking mode
  if (m_tracking == TRACK_BUTTON)
  {
    int hot = HitTest(pt);
    if ((hot >= 0) && (pt != m_mouse))
      GetToolBarCtrl().SetHotItem(hot);
    return;
  }
  m_mouse = pt;
  CToolBar::OnMouseMove(nFlags,pt);
}

void MenuBar::OnCustomDraw(NMHDR* nmhdr, LRESULT* result)
{
  NMTBCUSTOMDRAW* nmtbcd = (NMTBCUSTOMDRAW*)nmhdr;
  *result = CDRF_DODEFAULT;

  switch (nmtbcd->nmcd.dwDrawStage)
  {
  case CDDS_PREPAINT:
    *result = CDRF_NOTIFYITEMDRAW;
    break;
  case CDDS_ITEMPREPAINT:
    if (::IsAppThemed())
    {
      HTHEME theme = ::OpenThemeData(GetSafeHwnd(),L"Menu");
      if (theme)
      {
        int state = MBI_NORMAL;
        if ((nmtbcd->nmcd.uItemState & CDIS_HOT) != 0)
          state = m_popupMenu ? MBI_PUSHED : MBI_HOT;

        CDC* dc = CDC::FromHandle(nmtbcd->nmcd.hdc);
        CRect r(nmtbcd->nmcd.rc);
        ::DrawThemeBackground(theme,dc->GetSafeHdc(),MENU_BARITEM,state,r,NULL);

        CStringW text(GetButtonText((int)nmtbcd->nmcd.dwItemSpec));
        CFont* oldFont = dc->SelectObject(GetFont());
        DWORD txtFlags = DT_CENTER|DT_VCENTER|DT_SINGLELINE;
        if (SendMessage(WM_QUERYUISTATE) & UISF_HIDEACCEL)
          txtFlags |= DT_HIDEPREFIX;
        ::DrawThemeText(theme,dc->GetSafeHdc(),MENU_BARITEM,state,
          text,text.GetLength(),txtFlags,0,r);
        dc->SelectObject(oldFont);
        ::CloseThemeData(theme);

        *result = CDRF_SKIPDEFAULT;
      }
    }

    if (*result == CDRF_DODEFAULT)
    {
      *result = TBCDRF_NOEDGES|TBCDRF_NOOFFSET|TBCDRF_HILITEHOTTRACK;
      nmtbcd->clrHighlightHotTrack = ::GetSysColor(COLOR_MENUHILIGHT);
      if ((nmtbcd->nmcd.uItemState & CDIS_HOT) != 0)
        nmtbcd->clrText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
    }
    break;
  }
}

void MenuBar::OnDropDown(NMHDR* nmhdr, LRESULT* result)
{
  NMTOOLBAR* nmtb = (NMTOOLBAR*)nmhdr;
  *result = TBDDRET_DEFAULT;

  PostMessage(WM_MENUBAR_POPUP,nmtb->iItem,FALSE);
}

void MenuBar::OnHotItemChange(NMHDR* nmhdr, LRESULT* result)
{
  NMTBHOTITEM* nmtbhi = (NMTBHOTITEM*)nmhdr;
  *result = 0;

  if (m_tracking != TRACK_NONE)
  {
    // When tracking, don't allow mouse or keyboard events to set the hot button to -1
    if (nmtbhi->dwFlags & (HICF_ACCELERATOR|HICF_ARROWKEYS|HICF_DUPACCEL|HICF_MOUSE))
    {
      int button = (nmtbhi->dwFlags & HICF_LEAVING) ? -1 : nmtbhi->idNew;
      if (button == -1)
        *result = 1;
    }
  }
}

void MenuBar::OnUpdateButton(CCmdUI* pCmdUI)
{
  // Always enable the menu buttons
  pCmdUI->Enable(TRUE);
}

LRESULT MenuBar::OnMenuPopup(WPARAM wparam, LPARAM lparam)
{
  TrackPopupMenu((int)wparam,lparam != FALSE);
  return 1;
}

void MenuBar::OnUpdateCmdUI(CFrameWnd* target, BOOL disableIfNoHndler)
{
  if (m_tracking == TRACK_NONE)
  {
    BOOL always = FALSE;
    if (::SystemParametersInfo(SPI_GETKEYBOARDCUES,0,&always,0) == 0)
      always = TRUE;

    // Get the active frame window
    CFrameWnd* activeFrame = NULL;
    CWnd* active = CWnd::GetActiveWindow();
    if (active != NULL)
    {
      if (active->IsKindOf(RUNTIME_CLASS(CFrameWnd)))
        activeFrame = (CFrameWnd*)active;
      else
        activeFrame = active->GetParentFrame();
    }

    // Only respond to keys for the active frame window
    bool alt = false;
    bool f10 = false;
    if (activeFrame == GetParentFrame())
    {
      alt = ((::GetKeyState(VK_LMENU) & 0x8000) != 0);
      f10 = ((::GetKeyState(VK_F10) & 0x8000) != 0) && m_useF10;
    }

    // Show or hide the menu keyboard shortcuts
    SendMessage(WM_UPDATEUISTATE,
      MAKEWPARAM((always || alt || f10) ? UIS_CLEAR : UIS_SET,UISF_HIDEACCEL));
  }
  CToolBar::OnUpdateCmdUI(target,disableIfNoHndler);
}

BOOL MenuBar::OnMenuInput(MSG& msg)
{
  switch (msg.message)
  {
  case WM_KEYDOWN:
    if ((msg.wParam == VK_LEFT) && m_arrowLeft)
      TrackNewMenu(GetNextButton(m_popupTrack,true));
    else if ((msg.wParam == VK_RIGHT) && m_arrowRight)
      TrackNewMenu(GetNextButton(m_popupTrack,false));
    else if (msg.wParam == VK_ESCAPE)
      m_escapePressed = true;
    break;

  case WM_MOUSEMOVE:
    {
      // If the mouse has moved to a new menu, show it
      CPoint pt = msg.lParam;
      ScreenToClient(&pt);
      if (pt != m_mouse)
      {
        int button = HitTest(pt);
        if ((button >= 0) && (button != m_popupTrack))
          TrackNewMenu(button);
        m_mouse = pt;
      }
    }
    break;

  case WM_LBUTTONDOWN:
    {
      // If the button for the current menu is selected, cancel the menu
      CPoint pt = msg.lParam;
      ScreenToClient(&pt);
      if (HitTest(pt) == m_popupTrack)
      {
        TrackNewMenu(-1);
        return TRUE;
      }
    }
    break;
  }
  return FALSE;
}

void MenuBar::OnMenuSelect(HMENU menu, UINT flags)
{
  if (m_tracking != TRACK_NONE)
  {
    m_arrowRight = ((flags & MF_POPUP) == 0);
    m_arrowLeft = (menu == m_popupMenu);
  }
}

void MenuBar::TrackPopupMenu(int button, bool keyboard)
{
  while (button >= 0)
  {
    // Show the menu as selected in the menu bar
    m_popupNew = -1;
    GetToolBarCtrl().PressButton(button,TRUE);
    UpdateWindow();

    // If triggered by the keyboard, select the first item
    if (keyboard)
    {
      GetOwner()->PostMessage(WM_KEYDOWN,VK_DOWN,1);
      GetOwner()->PostMessage(WM_KEYUP,VK_DOWN,1);
    }

    SetTrackingState(TRACK_POPUP,button);
    Invalidate();

    // Trap menu input for keys and "hot tracking"
    menuBar = this;
    msgHook = ::SetWindowsHookEx(WH_MSGFILTER,InputFilter,NULL,::GetCurrentThreadId());

    // Show the menu
    CRect rect;
    GetToolBarCtrl().GetRect(button,rect);
    ClientToScreen(&rect);
    CMenu* menu = m_menu.GetSubMenu(button);
    m_popupMenu = menu->GetSafeHmenu();
    menu->TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,rect.left,rect.bottom,
      GetParentFrame(),&rect);
    m_popupMenu = 0;

    // Remove the input hook
    ::UnhookWindowsHookEx(msgHook);
    msgHook = NULL;
    menuBar = NULL;

    GetToolBarCtrl().PressButton(button,FALSE);
    UpdateWindow();

    SetTrackingState(m_escapePressed ? TRACK_BUTTON : TRACK_NONE,button);

    // If not -1, then the user has moved to another menu
    button = m_popupNew;
  }
}

void MenuBar::SetTrackingState(TrackingState track, int button)
{
  if (track == m_tracking)
    return;

  if (track == TRACK_NONE)
    button = -1;
  GetToolBarCtrl().SetHotItem(button);

  // Start tracking a menu
  if (track == TRACK_POPUP)
  {
    m_escapePressed = false;
    m_arrowRight = true;
    m_arrowLeft = true;
    m_popupTrack = button;
  }

  m_tracking = track;
}

void MenuBar::TrackNewMenu(int button)
{
  if (button != m_popupTrack)
  {
    // Cancel the current menu
    GetParentFrame()->PostMessage(WM_CANCELMODE);

    // Store the index of the new menu to show
    m_popupNew = button;
  }
}

int MenuBar::HitTest(CPoint pt)
{
  int hit = GetToolBarCtrl().HitTest(&pt);
  if ((hit >= 0) && (hit < GetToolBarCtrl().GetButtonCount()))
  {
    // Check that the hit button is visible
    CRect rect;
    GetClientRect(&rect);
    if (rect.PtInRect(pt))
      return hit;
  }
  return -1;
}

int MenuBar::GetNextButton(int button, bool goBack)
{
  if (goBack)
  {
    button--;
    if (button < 0)
      button = GetToolBarCtrl().GetButtonCount()-1;
  }
  else
  {
    button++;
    if (button >= GetToolBarCtrl().GetButtonCount())
      button = 0;
  }
  return button;
}

void MenuBar::SetBitmaps(CMenu* menu)
{
  bool hasBitmap = false;
  for (UINT i = 0; i < (UINT)menu->GetMenuItemCount(); i++)
  {
    UINT id = menu->GetMenuItemID(i);
    switch (id)
    {
    case -1: // Sub-menu
      SetBitmaps(menu->GetSubMenu(i));
      break;
    case 0: // Separator
      break;
    default:
      {
        Bitmap bitmap;
        if (m_bitmaps.Lookup(id,bitmap))
        {
          MENUITEMINFO mii = { sizeof MENUITEMINFO,0 };
          mii.fMask = MIIM_BITMAP;
          mii.hbmpItem = bitmap.bitmap;
          menu->SetMenuItemInfo(i,&mii,TRUE);
          hasBitmap = true;
        }
      }
      break;
    }
  }

  if (hasBitmap)
  {
    MENUINFO mi= { sizeof(MENUINFO),0 };
    mi.fMask = MIM_STYLE;
    ::GetMenuInfo(menu->GetSafeHmenu(),&mi);
    mi.dwStyle |= MNS_CHECKORBMP;
    ::SetMenuInfo(menu->GetSafeHmenu(),&mi);
  }
}

bool MenuBar::AllowAltX(WPARAM wp)
{
  if (m_filterAltX != NULL)
    return (*m_filterAltX)((char)wp);
  return true;
}

LRESULT CALLBACK MenuBar::InputFilter(int code, WPARAM wp, LPARAM lp)
{
  // Intercept any menu related messages
  if ((code == MSGF_MENU) && menuBar)
  {
    if (menuBar->OnMenuInput(*((MSG*)lp)))
      return TRUE;
  }
  return ::CallNextHookEx(msgHook,code,wp,lp);
}

MenuBar::Bitmap::Bitmap() : size(0,0)
{
  bitmap = 0;
  bits = NULL;
  initialBits = NULL;
}

MenuBar::Bitmap::Bitmap(HBITMAP bitmap_, SIZE size_, DWORD* bits_, DWORD* initialBits_) : size(size_)
{
  bitmap = bitmap_;
  bits = bits_;
  initialBits = initialBits_;
}

// Internal CToolBar data structure
struct CToolBarData
{
  WORD wVersion;
  WORD wWidth;
  WORD wHeight;
  WORD wItemCount;

  WORD* items()
  {
    return (WORD*)(this+1);
  }
};

BOOL BitmapToolBar::LoadToolBar(UINT id)
{
  return LoadToolBar(MAKEINTRESOURCE(id));
}

BOOL BitmapToolBar::LoadToolBar(LPCTSTR resName)
{
  HINSTANCE inst = AfxFindResourceHandle(resName,RT_TOOLBAR);
  HRSRC rsrc = ::FindResource(inst,resName,RT_TOOLBAR);
  if (rsrc == NULL)
  {
    inst = AfxGetInstanceHandle();
    rsrc = ::FindResource(inst,resName,RT_TOOLBAR);
  }
  if (rsrc == NULL)
  {
    inst = ::GetModuleHandle(NULL);
    rsrc = ::FindResource(inst,resName,RT_TOOLBAR);
  }
  if (rsrc == NULL)
    return FALSE;

  HGLOBAL globalRes = LoadResource(inst,rsrc);
  if (globalRes == NULL)
    return FALSE;

  CToolBarData* data = (CToolBarData*)LockResource(globalRes);
  if (data == NULL)
    return FALSE;
  ASSERT(data->wVersion == 1);

  UINT* items = new UINT[data->wItemCount];
  for (int i = 0; i < data->wItemCount; i++)
    items[i] = data->items()[i];
  BOOL result = SetButtons(items,data->wItemCount);
  delete[] items;

  if (result)
  {
    CSize sizeImage(data->wWidth,data->wHeight);
    CSize sizeButton(data->wWidth+7,data->wHeight+7);
    SetSizes(sizeButton,sizeImage);
    result = LoadBitmap(resName);
  }

  ::UnlockResource(globalRes);
  ::FreeResource(globalRes);
  return result;
}

BOOL BitmapToolBar::LoadBitmap(LPCTSTR resName)
{
  HINSTANCE inst = AfxFindResourceHandle(resName,RT_BITMAP);
  HRSRC rsrc = ::FindResource(inst,resName,RT_BITMAP);
  if (rsrc == NULL)
  {
    inst = AfxGetInstanceHandle();
    rsrc = ::FindResource(inst,resName,RT_BITMAP);
  }
  if (rsrc == NULL)
  {
    inst = ::GetModuleHandle(NULL);
    rsrc = ::FindResource(inst,resName,RT_BITMAP);
  }
  if (rsrc == NULL)
    return FALSE;

  HBITMAP image = AfxLoadSysColorBitmap(inst,rsrc);
  if (!AddReplaceBitmap(image))
    return FALSE;

  m_hInstImageWell = inst;
  m_hRsrcImageWell = rsrc;
  return TRUE;
}

void BitmapToolBar::SetButtonStyle(int idx, UINT style)
{
  TBBUTTON button;
  _GetButton(idx,&button);
  if (button.fsStyle != (BYTE)LOWORD(style) || button.fsState != (BYTE)HIWORD(style))
  {
    button.fsStyle = (BYTE)LOWORD(style);
    button.fsState = (BYTE)HIWORD(style);
    _SetButtonNoRecalc(idx,&button);
    m_bDelayedButtonLayout = TRUE;
  }
}

void BitmapToolBar::_SetButtonNoRecalc(int idx, TBBUTTON* newButton)
{
  TBBUTTON button;
  VERIFY(DefWindowProc(TB_GETBUTTON,idx,(LPARAM)&button));

  button.bReserved[0] = 0;
  button.bReserved[1] = 0;
  newButton->fsState ^= TBSTATE_ENABLED;
  newButton->bReserved[0] = 0;
  newButton->bReserved[1] = 0;

  if (memcmp(newButton,&button,sizeof(TBBUTTON)) != 0)
  {
    DWORD style = GetStyle();
    ModifyStyle(WS_VISIBLE,0);
    VERIFY(DefWindowProc(TB_DELETEBUTTON,idx,0));
    VERIFY(DefWindowProc(TB_INSERTBUTTON,idx,(LPARAM)newButton));
    ModifyStyle(0,style & WS_VISIBLE);

    if (((newButton->fsStyle ^ button.fsStyle) & TBSTYLE_SEP) ||
        ((newButton->fsStyle & TBSTYLE_SEP) && newButton->iBitmap != button.iBitmap))
    {
      Invalidate();
    }
    else
    {
      CRect rect;
      if (DefWindowProc(TB_GETITEMRECT,idx,(LPARAM)&rect))
        InvalidateRect(rect);
    }
  }
}

class BitmapToolBarCmdUI : public CCmdUI
{
public:
  virtual void Enable(BOOL on);
  virtual void SetCheck(int check);
  virtual void SetText(LPCTSTR) {}
};

void BitmapToolBarCmdUI::Enable(BOOL on)
{
  m_bEnableChanged = TRUE;
  BitmapToolBar* toolBar = (BitmapToolBar*)m_pOther;

  UINT newStyle = toolBar->GetButtonStyle(m_nIndex) & ~TBBS_DISABLED;
  if (!on)
  {
    newStyle |= TBBS_DISABLED;
    newStyle &= ~TBBS_PRESSED;
  }
  toolBar->SetButtonStyle(m_nIndex,newStyle);
}

void BitmapToolBarCmdUI::SetCheck(int check)
{
  BitmapToolBar* toolBar = (BitmapToolBar*)m_pOther;

  UINT newStyle = toolBar->GetButtonStyle(m_nIndex) & ~(TBBS_CHECKED|TBBS_INDETERMINATE);
  if (check == 1)
    newStyle |= TBBS_CHECKED;
  else if (check == 2)
    newStyle |= TBBS_INDETERMINATE;
  toolBar->SetButtonStyle(m_nIndex,newStyle|TBBS_CHECKBOX);
}

void BitmapToolBar::OnUpdateCmdUI(CFrameWnd* target, BOOL disableIfNo)
{
  BitmapToolBarCmdUI state;
  state.m_pOther = this;

  state.m_nIndexMax = (UINT)DefWindowProc(TB_BUTTONCOUNT,0,0);
  for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; state.m_nIndex++)
  {
    TBBUTTON button;
    _GetButton(state.m_nIndex,&button);
    state.m_nID = button.idCommand;

    if (!(button.fsStyle & TBSTYLE_SEP))
    {
      if (CWnd::OnCmdMsg(0,
        MAKELONG(CN_UPDATE_COMMAND_UI & 0xffff,WM_COMMAND+WM_REFLECT_BASE),&state,NULL))
        continue;
      if (CWnd::OnCmdMsg(state.m_nID,CN_UPDATE_COMMAND_UI,&state,NULL))
        continue;
      state.DoUpdate(target,disableIfNo);
    }
  }
  UpdateDialogControls(target,disableIfNo);
}

IMPLEMENT_DYNAMIC(MenuBarFrameWnd, CFrameWnd)

BEGIN_MESSAGE_MAP(MenuBarFrameWnd, CFrameWnd)
  ON_WM_MENUSELECT()
  ON_WM_MEASUREITEM()
  ON_WM_DRAWITEM()
  ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

MenuBarFrameWnd::MenuBarFrameWnd()
{
  m_menuBarIndex = -1;
  m_toolBarIndex = -1;
}

void MenuBarFrameWnd::UpdateDPI(int dpi)
{
  m_settings = Settings(dpi);

  // Resize the toolbar
  ImagePNG normalImage, disabledImage;
  if (m_toolBar.GetSafeHwnd() != 0)
  {
    if (m_image.Pixels())
    {
      CSize scaledSize(m_settings.sizeImage);
      scaledSize.cx *= m_toolBar.GetCount();
      normalImage.Scale(m_image,scaledSize);
      disabledImage.Copy(normalImage);

      normalImage.Fill(m_settings.colourFore);
      disabledImage.Fill(m_settings.colourDisable);
    }
    if (normalImage.Pixels() && disabledImage.Pixels())
    {
      m_toolBar.SetSizes(m_settings.sizeButton,m_settings.sizeImage);
      LoadBitmaps(normalImage,disabledImage);
    }
  }

  // Resize the menu bar
  if (m_menuBar.GetSafeHwnd() != 0)
  {
    if (normalImage.Pixels())
    {
      ASSERT(m_toolBar.GetSafeHwnd() != 0);
      m_menuBar.DeleteBitmaps();
      m_menuBar.LoadBitmaps(normalImage,m_toolBar.GetToolBarCtrl(),m_settings.sizeImage);
    }
    m_menuBar.UpdateFont(dpi);
    m_menuBar.Update();
  }
  SetBarSizes();
}

int MenuBarFrameWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  m_settings = Settings(DPI::getWindowDPI(this));
  return CFrameWnd::OnCreate(lpCreateStruct);
}

void MenuBarFrameWnd::OnMenuSelect(UINT id, UINT flags, HMENU menu)
{
#if (_MFC_VER == 0x700) // MFC bug
  menu = ((CMenu*)menu)->GetSafeHmenu();
#endif

  if (m_menuBar.GetSafeHwnd() != 0)
    m_menuBar.OnMenuSelect(menu,flags);
  CFrameWnd::OnMenuSelect(id,flags,menu);
}

void MenuBarFrameWnd::OnSettingChange(UINT uiAction, LPCTSTR lpszSection)
{
  CFrameWnd::OnSettingChange(uiAction,lpszSection);

  int dpi = DPI::getWindowDPI(this);
  if (m_settings != Settings(dpi))
    UpdateDPI(dpi);
}

BOOL MenuBarFrameWnd::PreTranslateMessage(MSG* msg)
{
  if (m_menuBar.GetSafeHwnd() != 0)
  {
    if ((msg->hwnd == GetSafeHwnd()) || IsChild(CWnd::FromHandle(msg->hwnd)))
    {
      if (m_menuBar.TranslateFrameMessage(msg))
        return TRUE;
    }
  }
  return CFrameWnd::PreTranslateMessage(msg);
}

BOOL MenuBarFrameWnd::CreateMenuBar(UINT id, CMenu* menu)
{
  if (!m_menuBar.Create(id,menu,this))
    return FALSE;

  // Create the cool bar with an appropriate style
  DWORD style = WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
  style |= (m_menuBar.GetSafeHwnd() != 0) ? CBRS_ALIGN_TOP : CBRS_TOP;
  if (!m_coolBar.Create(this,0,style))
    return FALSE;

  // Add the menus and toolbar
  if (m_menuBar.GetSafeHwnd() != 0)
  {
    // Only add the menu bar if it was created. If not, we are just using
    // the ordinary Windows menus.
    if (!m_coolBar.AddBar(&m_menuBar,NULL,NULL,RBBS_NOGRIPPER))
      return FALSE;
    m_menuBarIndex = m_coolBar.GetReBarCtrl().GetBandCount()-1;
  }
  return TRUE;
}

BOOL MenuBarFrameWnd::CreateBar(UINT id, UINT id32)
{
  CDC* dc = GetDC();
  int depth = dc->GetDeviceCaps(BITSPIXEL);
  ReleaseDC(dc);
  if (depth < 32)
    id32 = (UINT)-1;

  // Create the toolbar and load the resource for it
  if (!m_toolBar.CreateEx(this,TBSTYLE_FLAT|TBSTYLE_TRANSPARENT,
    WS_CHILD|WS_VISIBLE|CBRS_ALIGN_TOP|CBRS_TOOLTIPS|CBRS_FLYBY))
    return FALSE;
  if (!m_toolBar.LoadToolBar(id))
    return FALSE;

  // If given a 32-bit bitmap, add it to the toolbar
  if (id32 != -1)
  {
    CBitmap tbarBitmap;
    LoadBitmap(tbarBitmap,id32);
    if (!m_toolBar.SetBitmap((HBITMAP)tbarBitmap.Detach()))
      return FALSE;
  }

  // Load the bitmap again for the menu icons
  CSize sizeImage(16,15);
  CBitmap menuBitmap;
  LoadBitmap(menuBitmap,id32 != -1 ? id32 : id);
  m_menuBar.LoadBitmaps(menuBitmap,m_toolBar.GetToolBarCtrl(),sizeImage,id32 != -1);
  if (!m_menuBar.Create(id,0,this))
    return FALSE;

  // Create the cool bar with an appropriate style
  DWORD style = WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
  style |= (m_menuBar.GetSafeHwnd() != 0) ? CBRS_ALIGN_TOP : CBRS_TOP;
  if (!m_coolBar.Create(this,0,style))
    return FALSE;

  // Add the menus and toolbar
  if (m_menuBar.GetSafeHwnd() != 0)
  {
    // Only add the menu bar if it was created. If not, we are just using
    // the ordinary Windows menus.
    if (!m_coolBar.AddBar(&m_menuBar,NULL,NULL,RBBS_NOGRIPPER))
      return FALSE;
    m_menuBarIndex = m_coolBar.GetReBarCtrl().GetBandCount()-1;
  }
  if (!m_coolBar.AddBar(&m_toolBar,NULL,NULL,RBBS_NOGRIPPER|RBBS_BREAK))
    return FALSE;
  m_toolBarIndex = m_coolBar.GetReBarCtrl().GetBandCount()-1;
  return TRUE;
}

BOOL MenuBarFrameWnd::CreateNewBar(UINT id, UINT imageId)
{
  // Create the toolbar and load the resource for it
  if (!m_toolBar.CreateEx(this,TBSTYLE_FLAT|TBSTYLE_TRANSPARENT,
    WS_CHILD|WS_VISIBLE|CBRS_ALIGN_TOP|CBRS_TOOLTIPS|CBRS_FLYBY))
    return FALSE;
  if (!m_toolBar.LoadToolBar(id))
    return FALSE;

  // Scale sizes for the DPI
  m_settings = Settings(DPI::getWindowDPI(this));
  m_toolBar.SetSizes(m_settings.sizeButton,m_settings.sizeImage);

  // Load the image bitmap
  if (!m_image.LoadResource(imageId))
    return FALSE;

  // Scale the image bitmap
  ImagePNG normalImage, disabledImage;
  CSize scaledSize(m_settings.sizeImage);
  scaledSize.cx *= m_toolBar.GetCount();
  normalImage.Scale(m_image,scaledSize);
  disabledImage.Copy(normalImage);

  // Colour the image bitmaps appropriately
  normalImage.Fill(m_settings.colourFore);
  disabledImage.Fill(m_settings.colourDisable);

  // Add the scaled bitmap to the toolbar and menu
  LoadBitmaps(normalImage,disabledImage);
  m_menuBar.LoadBitmaps(normalImage,m_toolBar.GetToolBarCtrl(),m_settings.sizeImage);

  // Create the menu bar
  if (!m_menuBar.Create(id,0,this))
    return FALSE;

  // Create the cool bar with an appropriate style
  DWORD style = WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
  style |= (m_menuBar.GetSafeHwnd() != 0) ? CBRS_ALIGN_TOP : CBRS_TOP;
  if (!m_coolBar.Create(this,0,style))
    return FALSE;

  // Add the menus and toolbar
  if (!m_coolBar.AddBar(&m_menuBar,NULL,NULL,RBBS_NOGRIPPER))
    return FALSE;
  m_menuBarIndex = m_coolBar.GetReBarCtrl().GetBandCount()-1;
  if (!m_coolBar.AddBar(&m_toolBar,NULL,NULL,RBBS_NOGRIPPER))
    return FALSE;
  m_toolBarIndex = m_coolBar.GetReBarCtrl().GetBandCount()-1;

  SetBarSizes();
  return TRUE;
}

void MenuBarFrameWnd::LoadBitmaps(ImagePNG& normal, ImagePNG& disabled)
{
  m_toolBar.SetBitmap(normal.CopyBitmap(this));
  HIMAGELIST disabledList = ::ImageList_Create(
    m_settings.sizeImage.cx,m_settings.sizeImage.cy,ILC_COLOR32,0,5);
  if (disabledList)
  {
    if (::ImageList_Add(disabledList,disabled.CopyBitmap(this),0) >= 0)
      m_toolBar.GetToolBarCtrl().SetDisabledImageList(CImageList::FromHandle(disabledList));
  }
}

CMenu* MenuBarFrameWnd::GetMenu(void) const
{
  if (m_menuBar.GetSafeHwnd() != 0)
    return m_menuBar.GetMenu();
  return CFrameWnd::GetMenu();
}

void MenuBarFrameWnd::LoadBitmap(CBitmap& bitmap, UINT id)
{
  bitmap.LoadBitmap(id);
  if (bitmap.GetSafeHandle() == 0)
    bitmap.Attach(::LoadBitmap(::GetModuleHandle(NULL),MAKEINTRESOURCE(id)));
}

void MenuBarFrameWnd::SetBarSizes(void)
{
  REBARBANDINFO bandInfo = { sizeof(REBARBANDINFO),0 };
  bandInfo.fMask = RBBIM_CHILDSIZE;

  if (m_menuBar.GetSafeHwnd() != 0)
  {
    SIZE size;
    m_menuBar.SendMessage(TB_GETMAXSIZE,0,(LPARAM)&size);
    int dpi = DPI::getWindowDPI(this);
    size.cx += DPI::getSystemMetrics(SM_CXMENUCHECK,dpi);

    bandInfo.cxMinChild = size.cx;
    bandInfo.cyMinChild = size.cy;
    m_coolBar.GetReBarCtrl().SetBandInfo(m_menuBarIndex,&bandInfo);
  }

  if (m_toolBar.GetSafeHwnd() != 0)
  {
    CSize size = m_toolBar.CalcFixedLayout(FALSE,TRUE);

    bandInfo.cxMinChild = size.cx;
    bandInfo.cyMinChild = size.cy;
    m_coolBar.GetReBarCtrl().SetBandInfo(m_toolBarIndex,&bandInfo);
  }

  if (m_menuBar.GetSafeHwnd() != 0)
    m_coolBar.GetReBarCtrl().MinimizeBand(m_menuBarIndex);
}

MenuBarFrameWnd::Settings::Settings()
{
  menuY = 0;
  menuFontHeight = 0;
  colourFore = 0;
  colourDisable = 0;
}

MenuBarFrameWnd::Settings::Settings(int dpi)
{
  menuY = DPI::getSystemMetrics(SM_CYMENU,dpi);
  CFont font;
  if (DPI::createSystemMenuFont(&font,dpi))
  {
    LOGFONT lf;
    font.GetLogFont(&lf);
    menuFontHeight = lf.lfHeight;
  }

  sizeImage.cx = DPI::getSystemMetrics(SM_CXMENUCHECK,dpi);
  if (sizeImage.cx < 16)
    sizeImage.cx = 16;
  sizeImage.cy = DPI::getSystemMetrics(SM_CYMENUCHECK,dpi);
  if (sizeImage.cy < 16)
    sizeImage.cy = 16;
  sizeButton.cx = (sizeImage.cx*3)/2;
  sizeButton.cy = (sizeImage.cy*3)/2;

  // Replace black with dark grey
  colourFore = ::GetSysColor(COLOR_BTNTEXT);
  if (colourFore == RGB(0,0,0))
    colourFore = RGB(64,64,64);
  colourDisable = ::GetSysColor(COLOR_GRAYTEXT);
}

bool MenuBarFrameWnd::Settings::operator!=(const Settings& set) const
{
  if (menuY != set.menuY)
    return true;
  if (menuFontHeight != set.menuFontHeight)
    return true;
  if (sizeImage != set.sizeImage)
    return true;
  if (sizeButton != set.sizeButton)
    return true;
  if (colourFore != set.colourFore)
    return true;
  if (colourDisable != set.colourDisable)
    return true;
  return false;
}
