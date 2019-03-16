#pragma once

#ifndef NO_PNG
#include "ImagePNG.h"
#endif

#define WM_MENUBAR_POPUP (WM_APP+1000)

typedef BOOL (WINAPI* GETMENUINFO)(HMENU,LPMENUINFO);
typedef BOOL (WINAPI* SETMENUINFO)(HMENU,LPCMENUINFO);

class MenuBar : public CToolBar
{
public:
  typedef bool (*FilterAltX)(char c);

  MenuBar();
  ~MenuBar();

  void SetUseF10(bool use);
  void SetFilterAltX(FilterAltX filter);
  BOOL Create(UINT id, CMenu* menu, CWnd* parent);
  void LoadBitmaps(CBitmap& bitmap, CToolBarCtrl& bar, CSize size, bool alpha);
  void DeleteBitmaps(void);
  void Update(void);
  void UpdateFont(int dpi);
  CMenu* GetMenu(void) const;
  DWORD GetOS(void) const;

  BOOL TranslateFrameMessage(MSG* msg);
  void OnMenuSelect(HMENU menu, UINT flags);
  void OnMeasureItem(LPMEASUREITEMSTRUCT mis);
  void OnDrawItem(LPDRAWITEMSTRUCT mis);

protected: 
  DECLARE_DYNAMIC(MenuBar)

  afx_msg void OnMouseMove(UINT, CPoint);
  afx_msg void OnCustomDraw(NMHDR*, LRESULT*);
  afx_msg void OnDropDown(NMHDR*, LRESULT*);
  afx_msg void OnHotItemChange(NMHDR*, LRESULT*);
  afx_msg void OnUpdateButton(CCmdUI*);
  afx_msg LRESULT OnMenuPopup(WPARAM, LPARAM);
  DECLARE_MESSAGE_MAP()

  void OnUpdateCmdUI(CFrameWnd*, BOOL);
  BOOL OnMenuInput(MSG& msg);

  enum TrackingState
  {
    TRACK_NONE,   // Not tracking anything
    TRACK_BUTTON, // Tracking buttons (F10/Alt mode)
    TRACK_POPUP   // Tracking popup menus
  };

  void TrackPopupMenu(int button, bool keyboard);
  void SetTrackingState(TrackingState track, int button = -1);
  void TrackNewMenu(int button);
  int HitTest(CPoint pt);
  int GetNextButton(int button, bool goBack);
  void SetBitmaps(CMenu* menu);
  bool AllowAltX(WPARAM wp);

  static LRESULT CALLBACK InputFilter(int code, WPARAM wp, LPARAM lp);

  DWORD m_os;
  GETMENUINFO m_getMenuInfo;
  SETMENUINFO m_setMenuInfo;
  bool m_useF10;
  FilterAltX m_filterAltX;

  CMenu m_menu;
  CFont m_font;

  struct Bitmap
  {
    Bitmap();
    Bitmap(HBITMAP, SIZE, DWORD*, DWORD*);

    HBITMAP bitmap;
    CSize size;
    DWORD* bits;
    DWORD* initialBits;
  };
  CMap<UINT,UINT,Bitmap,Bitmap&> m_bitmaps;

  TrackingState m_tracking;
  int m_popupTrack;
  int m_popupNew;
  HMENU m_popupMenu;
  bool m_arrowLeft;
  bool m_arrowRight;
  bool m_escapePressed;
  CPoint m_mouse;
};

class MenuBarFrameWnd : public CFrameWnd
{
public:
  MenuBarFrameWnd();
  void UpdateDPI(int dpi);

protected: 
  DECLARE_DYNAMIC(MenuBarFrameWnd)

  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnMenuSelect(UINT, UINT, HMENU);
  afx_msg void OnMeasureItem(int, LPMEASUREITEMSTRUCT);
  afx_msg void OnDrawItem(int, LPDRAWITEMSTRUCT);
  afx_msg void OnSettingChange(UINT, LPCTSTR);
  DECLARE_MESSAGE_MAP()

  BOOL PreTranslateMessage(MSG*);

  BOOL CreateMenuBar(UINT id, CMenu* menu);
  BOOL CreateBar(UINT id, UINT id32);
  CMenu* GetMenu(void) const;
  void LoadBitmap(CBitmap& bitmap, UINT id);
  void SetBarSizes(void);

  CReBar m_coolBar;
  MenuBar m_menuBar;
  CToolBar m_toolBar;

  int m_menuBarIndex;
  int m_toolBarIndex;

#ifndef NO_PNG
  BOOL CreateNewBar(UINT id, UINT imageId);

  ImagePNG m_image;
#endif

  struct Settings
  {
    int menuY;
    int menuFontHeight;
    CSize sizeImage;
    CSize sizeButton;
    COLORREF colourBack;
    COLORREF colourFore;

    Settings();
    Settings(int dpi);
    bool operator!=(const Settings& set) const;
  };

  Settings m_settings;
};
