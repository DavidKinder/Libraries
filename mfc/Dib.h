/////////////////////////////////////////////////////////////////////////////
//
// Dib
// Device independant bitmap classes
// Based on code taken from http://www.codeproject.com/
//
/////////////////////////////////////////////////////////////////////////////

#ifndef DIB_H_
#define DIB_H_

////////////////////////////////////////////////////////////////////////////////
// Declaration of the DIB class
////////////////////////////////////////////////////////////////////////////////

class CDib
{
public:
  CDib();
  ~CDib();

public:
  void LoadBitmap(UINT nID);

  BOOL IsOK(void);
  int GetWidth(void);
  int GetHeight(void);

  void DrawShaded(CDC* pDC, CPoint& Position, DWORD BackColour, double dFactor);
  void Draw(CDC* pDC, CPoint& Position);

protected:
  template<class T> static T PadWidth(T t) { return ((t*8+31)&(~31))/8; }

protected:
  BITMAPINFO *m_pInfo;
  BYTE *m_pPixels;
};

////////////////////////////////////////////////////////////////////////////////
// Declaration of the DIBSection class
////////////////////////////////////////////////////////////////////////////////

class CDibSection
{
public:
  CDibSection();
  ~CDibSection();

  HBITMAP GetSafeHandle(void) const;
  CSize GetSize(void) const;
  DWORD* GetBits(void) const;
  const BITMAPINFO* GetBitmapInfo(void) const;

  BOOL CreateBitmap(HDC hDC, LONG iWidth, LONG iHeight);
  void DeleteBitmap(void);

  void AlphaBlend(COLORREF back);
  void AlphaBlend(const CDibSection* from, LONG x, LONG y, BOOL invert = FALSE);
  void AlphaBlend(const CDibSection* from, LONG fx, LONG fy, LONG fw, LONG fh,
    LONG x, LONG y, BOOL invert);

  void FillSolid(COLORREF back);
  void BlendSolidRect(LPRECT rect, COLORREF colour, int alpha);
  void Darken(double dark);

  // Get and set the colour at (x,y). For speed reasons these
  // routines do no error checking.

  inline void SetPixel(LONG x, LONG y, DWORD col)
  {
    m_ppvBits[x+(y*m_BitmapInfo.bmiHeader.biWidth)] = col;
  }

  static inline void SetPixel(DWORD* ppvBits, LONG w, LONG x, LONG y, DWORD col)
  {
    ppvBits[x+(y*w)] = col;
  }

  inline DWORD GetPixel(LONG x, LONG y) const
  {
    return m_ppvBits[x+(y*m_BitmapInfo.bmiHeader.biWidth)];
  }

  static inline DWORD GetPixel(DWORD* ppvBits, LONG w, LONG x, LONG y)
  {
    return ppvBits[x+(y*w)];
  }

  COLORREF GetPixelColour(LONG x, LONG y) const;

  static CBitmap* SelectDibSection(CDC& DC, CDibSection* pDibSection);

protected:
  HBITMAP m_hBitmap;
  BITMAPINFO m_BitmapInfo;
  DWORD* m_ppvBits;
};

#endif // DIB_H_
