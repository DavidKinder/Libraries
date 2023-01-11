#pragma once

#include "DarkMode.h"
#include "ImagePNG.h"

#define WM_MENUBAR_POPUP (WM_APP+1000)

class MenuBar : public CToolBar
{
public:
  typedef bool (*FilterAltX)(char c);

  MenuBar();
  ~MenuBar();

  void SetUseF10(bool use);
  void SetFilterAltX(FilterAltX filter);
  void AddNoIconId(UINT id);
  BOOL Create(UINT id, CMenu* menu, CWnd* parent);
  void LoadBitmaps(CBitmap& bitmap, CToolBarCtrl& bar, CSize size, bool alpha);
  void DeleteBitmaps(void);
  void Update(void);
  void UpdateFont(int dpi);
  CMenu* GetMenu(void) const;
  DWORD GetOS(void) const;

  void LoadBitmaps(ImagePNG& images, CToolBarCtrl& bar, CSize size);

  BOOL TranslateFrameMessage(MSG* msg);
  void OnMenuSelect(HMENU menu, UINT flags);

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

  bool m_useF10;
  FilterAltX m_filterAltX;
  CArray<int> m_noIconIds;

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

class BitmapToolBar : public CToolBar
{
public:
  BitmapToolBar()
  {
  }

  BOOL LoadToolBar(UINT id);
  BOOL LoadToolBar(LPCTSTR resName);
  BOOL LoadBitmap(LPCTSTR resName);

  void SetButtonStyle(int idx, UINT style);

  virtual void OnUpdateCmdUI(CFrameWnd* target, BOOL disableIfNo);

protected:
  void _SetButtonNoRecalc(int idx, TBBUTTON* newButton);
};

class MenuBarFrameWnd : public CFrameWnd
{
public:
  MenuBarFrameWnd();
  virtual ~MenuBarFrameWnd();

  void UpdateDPI(int dpi);
  virtual void SetDarkMode(DarkMode* dark);

protected: 
  DECLARE_DYNAMIC(MenuBarFrameWnd)

  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnMenuSelect(UINT, UINT, HMENU);
  afx_msg void OnSettingChange(UINT, LPCTSTR);
  afx_msg LRESULT OnDarkModeActive(WPARAM, LPARAM);
  DECLARE_MESSAGE_MAP()

  BOOL PreTranslateMessage(MSG*);

  BOOL CreateMenuBar(UINT id, CMenu* menu);
  BOOL CreateBar(UINT id, UINT id32);
  CMenu* GetMenu(void) const;
  void LoadBitmap(CBitmap& bitmap, UINT id);
  void SetBarSizes(void);

  BOOL CreateNewBar(UINT id, UINT imageId);
  void LoadBitmaps(ImagePNG& normal, ImagePNG& disabled);

  CReBar m_coolBar;
  MenuBar m_menuBar;
  BitmapToolBar m_toolBar;

  int m_menuBarIndex;
  int m_toolBarIndex;

  ImagePNG m_image;

  struct Settings
  {
    int menuY;
    int menuFontHeight;
    CSize sizeImage;
    CSize sizeButton;
    COLORREF colourFore;
    COLORREF colourDisable;

    Settings();
    Settings(int dpi);
    bool operator!=(const Settings& set) const;
  };

  Settings m_settings;
  DarkMode* m_dark;
};
