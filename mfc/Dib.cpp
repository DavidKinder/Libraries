/////////////////////////////////////////////////////////////////////////////
//
// Dib
// Device independant bitmap classes
// Based on code taken from http://www.codeproject.com/
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Dib.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////////////////
// Implementation of the DIB class
////////////////////////////////////////////////////////////////////////////////

CDib::CDib()
{
  m_pInfo = NULL;
  m_pPixels = NULL;
}

CDib::~CDib()
{
  if (m_pInfo)
    delete[] (BYTE*)m_pInfo;
  if (m_pPixels)
    delete[] m_pPixels;
}

void CDib::LoadBitmap(UINT nID)
{
  HBITMAP hBitmap = (HBITMAP)::LoadImage(AfxGetInstanceHandle(),
    MAKEINTRESOURCE(nID),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
  if (hBitmap == 0)
    return;

  CBitmap Bitmap;
  Bitmap.Attach(hBitmap);

  BITMAP BitmapHeader;
  Bitmap.GetBitmap(&BitmapHeader);

  DWORD dwWidth;
  if (BitmapHeader.bmBitsPixel > 8)
    dwWidth = PadWidth(BitmapHeader.bmWidth*3);
  else
    dwWidth = PadWidth(BitmapHeader.bmWidth);
  m_pPixels = new BYTE[dwWidth*BitmapHeader.bmHeight];

  WORD wColours = 0;
  switch(BitmapHeader.bmBitsPixel)
  {
  case 1:
    wColours = 2;
    break;
  case 4:
    wColours = 16;
    break;
  case 8:
    wColours = 256;
    break;
  }
  m_pInfo = (BITMAPINFO*)new BYTE[sizeof(BITMAPINFOHEADER) + wColours*sizeof(RGBQUAD)];

  m_pInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  m_pInfo->bmiHeader.biWidth = BitmapHeader.bmWidth;
  m_pInfo->bmiHeader.biHeight = BitmapHeader.bmHeight;
  m_pInfo->bmiHeader.biPlanes = BitmapHeader.bmPlanes;

  if( BitmapHeader.bmBitsPixel > 8)
    m_pInfo->bmiHeader.biBitCount = 24;
  else
    m_pInfo->bmiHeader.biBitCount = BitmapHeader.bmBitsPixel;

  m_pInfo->bmiHeader.biCompression = BI_RGB;
  m_pInfo->bmiHeader.biSizeImage = ((((BitmapHeader.bmWidth * BitmapHeader.bmBitsPixel) + 31) & ~31) >> 3) * BitmapHeader.bmHeight;
  m_pInfo->bmiHeader.biXPelsPerMeter = 0;
  m_pInfo->bmiHeader.biYPelsPerMeter = 0;
  m_pInfo->bmiHeader.biClrUsed = 0;
  m_pInfo->bmiHeader.biClrImportant = 0;

  CClientDC dc(CWnd::GetDesktopWindow());
  ::GetDIBits(dc.GetSafeHdc(),(HBITMAP)Bitmap.GetSafeHandle(),
     0,(WORD)BitmapHeader.bmHeight,m_pPixels,m_pInfo,DIB_RGB_COLORS);

  Bitmap.DeleteObject();
}

BOOL CDib::IsOK(void)
{
  return (m_pInfo && m_pPixels) ? TRUE : FALSE;
}

int CDib::GetWidth(void)
{
  int iWidth = 0;
  if (m_pInfo && m_pPixels)
    iWidth = m_pInfo->bmiHeader.biWidth;
  return iWidth;
}

int CDib::GetHeight(void)
{
  int iHeight = 0;
  if (m_pInfo && m_pPixels)
    iHeight = m_pInfo->bmiHeader.biHeight;
  return iHeight;
}

void CDib::DrawShaded(CDC* pDC, CPoint& Position, DWORD BackColour, double dFactor)
{
  if (IsOK())
  {
    WORD wColours = m_pInfo->bmiHeader.biClrUsed ?
      (WORD)m_pInfo->bmiHeader.biClrUsed : 
      (WORD)(1 << m_pInfo->bmiHeader.biBitCount);

    BYTE BackR = GetRValue(BackColour);
    BYTE BackG = GetGValue(BackColour);
    BYTE BackB = GetBValue(BackColour);

    RGBQUAD* pCopyCols = new RGBQUAD[wColours];
    for(int i = 0; i < wColours; i++)
      pCopyCols[i] = m_pInfo->bmiColors[i];

    for(int i = 0; i < wColours; i++)
    {
      double dSquareSum =
          m_pInfo->bmiColors[i].rgbRed 
        * m_pInfo->bmiColors[i].rgbRed
        + m_pInfo->bmiColors[i].rgbGreen 
        * m_pInfo->bmiColors[i].rgbGreen
        + m_pInfo->bmiColors[i].rgbBlue 
        * m_pInfo->bmiColors[i].rgbBlue;
      double dScale = sqrt(dSquareSum/(3.0*255.0*255.0));
      dScale = dFactor + (dScale*(1.0-dFactor));

      m_pInfo->bmiColors[i].rgbRed   = (BYTE)(BackR*dScale);
      m_pInfo->bmiColors[i].rgbGreen = (BYTE)(BackG*dScale);
      m_pInfo->bmiColors[i].rgbBlue  = (BYTE)(BackB*dScale);
    }

    Draw(pDC,Position);

    for(int i = 0; i < wColours; i++)
      m_pInfo->bmiColors[i] = pCopyCols[i];
    delete[] pCopyCols;
  }
}

void CDib::Draw(CDC* pDC, CPoint& Position)
{
  if (IsOK())
  {
    ::StretchDIBits(pDC->m_hDC,
      Position.x, Position.y,
      m_pInfo->bmiHeader.biWidth, m_pInfo->bmiHeader.biHeight,
      0,0,
      m_pInfo->bmiHeader.biWidth, m_pInfo->bmiHeader.biHeight,
      (LPVOID)m_pPixels,(LPBITMAPINFO)m_pInfo,
      DIB_RGB_COLORS,SRCCOPY);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Implementation of the DIBSection class
////////////////////////////////////////////////////////////////////////////////

CDibSection::CDibSection()
{
  m_hBitmap = 0;
  m_ppvBits = NULL;

  ::ZeroMemory(&m_BitmapInfo,sizeof(BITMAPINFO));
  m_BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  m_BitmapInfo.bmiHeader.biPlanes = 1;
  m_BitmapInfo.bmiHeader.biBitCount = 32;
  m_BitmapInfo.bmiHeader.biCompression = BI_RGB;
}

CDibSection::~CDibSection()
{
  if (m_hBitmap)
    ::DeleteObject(m_hBitmap);
}

HBITMAP CDibSection::GetSafeHandle(void) const
{
  return this ? m_hBitmap : 0;
}

CSize CDibSection::GetSize(void) const
{
  CSize Size(m_BitmapInfo.bmiHeader.biWidth,
    abs(m_BitmapInfo.bmiHeader.biHeight));
  return Size;
}

DWORD* CDibSection::GetBits(void) const
{
  return m_ppvBits;
}

const BITMAPINFO* CDibSection::GetBitmapInfo(void) const
{
  return &m_BitmapInfo;
}

BOOL CDibSection::CreateBitmap(HDC hDC, LONG iWidth, LONG iHeight)
{
  if (m_hBitmap)
    return FALSE;

  m_BitmapInfo.bmiHeader.biWidth = iWidth;
  m_BitmapInfo.bmiHeader.biHeight = iHeight * -1;

  m_hBitmap = ::CreateDIBSection(hDC,&m_BitmapInfo,DIB_RGB_COLORS,
    (VOID**)&m_ppvBits,NULL,0);
  if (m_hBitmap == 0)
    return FALSE;

  return TRUE;
}

void CDibSection::DeleteBitmap(void)
{
  if (m_hBitmap)
    ::DeleteObject(m_hBitmap);
  m_hBitmap = 0;
  m_ppvBits = NULL;
}

void CDibSection::AlphaBlend(COLORREF back)
{
  int sr, sg, sb, dr, dg, db, a;
  DWORD src;

  CSize size = GetSize();
  for (int y = 0; y < size.cy; y++)
  {
    for (int x = 0; x < size.cx; x++)
    {
      src = GetPixel(x,y);
      sb = src & 0xFF;
      src >>= 8;
      sg = src & 0xFF;
      src >>= 8;
      sr = src & 0xFF;
      src >>= 8;
      a = src & 0xFF;

      dr = GetRValue(back);
      dg = GetGValue(back);
      db = GetBValue(back);

      if (a == 0)
      {
      }
      else if (a == 0xFF)
      {
        dr = sr;
        dg = sg;
        db = sb;
      }
      else
      {
        // Rescale from 0..255 to 0..256
        a += a>>7;

        dr += (a * (sr - dr) >> 8);
        dg += (a * (sg - dg) >> 8);
        db += (a * (sb - db) >> 8);
      }

      SetPixel(x,y,(0xFF<<24)|(dr<<16)|(dg<<8)|db);
    }
  }
}

void CDibSection::AlphaBlend(const CDibSection* from, LONG x, LONG y, BOOL invert)
{
  CSize sz = from->GetSize();
  AlphaBlend(from,0,0,sz.cx,sz.cy,x,y,invert);
}

void CDibSection::AlphaBlend(const CDibSection* from, LONG fx, LONG fy, LONG fw, LONG fh, LONG x, LONG y, BOOL invert)
{
  int sr, sg, sb, dr, dg, db, a;
  DWORD src, dest;

  CSize srcSize = from->GetSize();
  CSize destSize = GetSize();

  for (int y1 = 0; y1 < fh; y1++)
  {
    for (int x1 = 0; x1 < fw; x1++)
    {
      if ((x+x1 < 0) || (x+x1 >= destSize.cx))
        continue;
      if ((y+y1 < 0) || (y+y1 >= destSize.cy))
        continue;

      dest = GetPixel(x+x1,y+y1);
      db = dest & 0xFF;
      dest >>= 8;
      dg = dest & 0xFF;
      dest >>= 8;
      dr = dest & 0xFF;

      src = invert ? from->GetPixel(fx+x1,srcSize.cy-fy-y1-1) : from->GetPixel(fx+x1,fy+y1);
      sb = src & 0xFF;
      src >>= 8;
      sg = src & 0xFF;
      src >>= 8;
      sr = src & 0xFF;
      src >>= 8;
      a = src & 0xFF;

      if (a == 0)
      {
      }
      else if (a == 0xFF)
      {
        dr = sr;
        dg = sg;
        db = sb;
      }
      else
      {
        // Rescale from 0..255 to 0..256
        a += a>>7;

        dr += (a * (sr - dr) >> 8);
        dg += (a * (sg - dg) >> 8);
        db += (a * (sb - db) >> 8);
      }

      SetPixel(x+x1,y+y1,(0xFF<<24)|(dr<<16)|(dg<<8)|db);
    }
  }
}

void CDibSection::FillSolid(COLORREF back)
{
  CSize size = GetSize();
  DWORD col = (0xFF<<24)|(GetRValue(back)<<16)|(GetGValue(back)<<8)|GetBValue(back);

  for (int y = 0; y < size.cy; y++)
  {
    for (int x = 0; x < size.cx; x++)
      SetPixel(x,y,col);
  }
}

void CDibSection::Darken(double dark)
{
  int sr, sg, sb, a;
  DWORD src;

  CSize size = GetSize();
  for (int y = 0; y < size.cy; y++)
  {
    for (int x = 0; x < size.cx; x++)
    {
      src = GetPixel(x,y);
      sb = src & 0xFF;
      src >>= 8;
      sg = src & 0xFF;
      src >>= 8;
      sr = src & 0xFF;
      src >>= 8;
      a = src & 0xFF;

      sr = (int)(sr*dark);
      sg = (int)(sg*dark);
      sb = (int)(sb*dark);

      SetPixel(x,y,(a<<24)|(sr<<16)|(sg<<8)|sb);
    }
  }
}

COLORREF CDibSection::GetPixelColour(LONG x, LONG y) const
{
  DWORD rgb = GetPixel(x,y);
  BYTE r = (BYTE)((rgb >> 16) & 0xFF);
  BYTE g = (BYTE)((rgb >> 8) & 0xFF);
  BYTE b = (BYTE)(rgb & 0xFF);
  return RGB(r,g,b);
}

CBitmap* CDibSection::SelectDibSection(CDC& DC, CDibSection* pDibSection)
{
  return CBitmap::FromHandle((HBITMAP)::SelectObject(
    DC.GetSafeHdc(),pDibSection->GetSafeHandle()));
}
