#include "stdafx.h"
#include "TextOutput.h"

// From wingdi.h
#ifndef GGI_MARK_NONEXISTING_GLYPHS
#define GGI_MARK_NONEXISTING_GLYPHS 1
#endif

namespace {
void AddTextExtent(HDC dc, LPCWSTR str, UINT count, CSize& result)
{
  SIZE sz = { 0,0 };
  if (::GetTextExtentPoint32W(dc,str,count,&sz))
  {
    result.cx += sz.cx;
    if (sz.cy > result.cy)
      result.cy = sz.cy;
  }
}

void ShiftYPos(HDC dc, int y)
{
  POINT pos;
  ::GetCurrentPositionEx(dc,&pos);
  ::MoveToEx(dc,pos.x,pos.y+y,NULL);
}
} // unnamed namespace

TextOutput::TextOutput()
{
  // Create the MLang COM object
  if (FAILED(m_fl.CoCreateInstance(CLSID_CMultiLanguage)))
    TRACE("Failed to create MLang object\n");

  // Open the GDI32 DLL and get function pointers from it
  m_gdi = ::LoadLibrary("gdi32.dll");
  ASSERT(m_gdi != 0);
  m_getGlyphIndicesW = (GetGlyphIndicesWPtr)::GetProcAddress(m_gdi,"GetGlyphIndicesW");
  m_getCharABCWidthsFloatW = (GetCharABCWidthsFloatWPtr)::GetProcAddress(m_gdi,"GetCharABCWidthsFloatW");
}

TextOutput::~TextOutput()
{
  Reset();
  ::FreeLibrary(m_gdi);
}

void TextOutput::TextOut(HDC dc, int x, int y, LPCWSTR str, UINT count)
{
  // Save and update the alignment and position
  UINT align = ::GetTextAlign(dc);
  POINT pos = { 0,0 };
  if (!(align & TA_UPDATECP))
  {
    ::SetTextAlign(dc,align|TA_UPDATECP);
    ::MoveToEx(dc,x,y,&pos);
  }

  // Get the default text metrics
  TEXTMETRIC metrics;
  ::GetTextMetrics(dc,&metrics);

  // If no font linking, just use the current font
  if (m_fl == NULL)
    goto leave;

  // Get the code pages of the current font, which will be used if possible
  HFONT font = (HFONT)::GetCurrentObject(dc,OBJ_FONT);
  DWORD fontCodePages = 0;
  HRESULT hr = m_fl->GetFontCodePages(dc,font,&fontCodePages);
  if (FAILED(hr))
    goto leave;

  while (count > 0)
  {
    // Get the next range that can be output in a font
    DWORD theCodePages = 0;
    long theCount = 0;
    hr = m_fl->GetStrCodePages(str,count,fontCodePages,&theCodePages,&theCount);
    if (FAILED(hr))
      goto leave;

    if (theCodePages & fontCodePages)
    {
      // Use the current font, as it has a suitable code page
      ::TextOutW(dc,0,0,str,theCount);
    }
    else
    {
      HFONT linkFont = 0;
      if (theCodePages == 0)
      {
        // No code page given, so try to map a single character to a font
        theCount = 1;
        hr = m_fl->MapFont(dc,0,*str,&linkFont);
      }
      else
      {
        // Get a font for the given code pages
        hr = m_fl->MapFont(dc,theCodePages,0,&linkFont);
      }

      if (SUCCEEDED(hr))
      {
        // Got a suitable font, so use it
        ::SelectObject(dc,linkFont);

        // Adjust for the font baseline
        int y = 0;
        if ((align & (TA_TOP|TA_BASELINE|TA_BOTTOM)) == TA_TOP)
        {
          TEXTMETRIC linkMetrics;
          ::GetTextMetrics(dc,&linkMetrics);
          y = linkMetrics.tmAscent - metrics.tmAscent;
        }
        if (y != 0)
          ShiftYPos(dc,-y);

        // Output the text in the linked font
        ::TextOutW(dc,0,0,str,theCount);

        // Put back the font baseline adjustment
        if (y != 0)
          ShiftYPos(dc,y);

        ::SelectObject(dc,font);
        m_fl->ReleaseFont(linkFont);
      }
      else
      {
        // Just use the current font
        ::TextOutW(dc,0,0,str,theCount);
      }
    }
    str += theCount;
    count -= theCount;
  }

leave:
  // If there is any text left to be output, use the current font
  if (count > 0)
    ::TextOutW(dc,0,0,str,count);

  // Put back the alignment and position
  if (!(align & TA_UPDATECP))
  {
    ::SetTextAlign(dc,align);
    ::MoveToEx(dc,pos.x,pos.y,NULL);
  }
}

void TextOutput::TextOut(HDC dc, int x, int y, LPCWSTR str, UINT count, const RECT& rect, bool opaque)
{
  // Save and update the alignment and position
  UINT align = ::GetTextAlign(dc);
  POINT pos = { 0,0 };
  if (!(align & TA_UPDATECP))
  {
    ::SetTextAlign(dc,align|TA_UPDATECP);
    ::MoveToEx(dc,x,y,&pos);
  }

  // If opaque, clear the rectangle
  if (opaque)
    ::ExtTextOutW(dc,0,0,ETO_OPAQUE,&rect,NULL,0,NULL);

  // Get the default text metrics
  TEXTMETRIC metrics;
  ::GetTextMetrics(dc,&metrics);

  // If no font linking, just use the current font
  if (m_fl == NULL)
    goto leave;

  // Get the code pages of the current font, which will be used if possible
  HFONT font = (HFONT)::GetCurrentObject(dc,OBJ_FONT);
  DWORD fontCodePages = 0;
  HRESULT hr = m_fl->GetFontCodePages(dc,font,&fontCodePages);
  if (FAILED(hr))
    goto leave;

  while (count > 0)
  {
    // Get the next range that can be output in a font
    DWORD theCodePages = 0;
    long theCount = 0;
    hr = m_fl->GetStrCodePages(str,count,fontCodePages,&theCodePages,&theCount);
    if (FAILED(hr))
      goto leave;

    if (theCodePages & fontCodePages)
    {
      // Use the current font, as it has a suitable code page
      ::ExtTextOutW(dc,0,0,0,&rect,str,theCount,NULL);
    }
    else
    {
      HFONT linkFont = 0;
      if (theCodePages == 0)
      {
        // No code page given, so try to map a single character to a font
        theCount = 1;
        hr = m_fl->MapFont(dc,0,*str,&linkFont);
      }
      else
      {
        // Get a font for the given code pages
        hr = m_fl->MapFont(dc,theCodePages,0,&linkFont);
      }

      if (SUCCEEDED(hr))
      {
        // Got a suitable font, so use it
        ::SelectObject(dc,linkFont);

        // Adjust for the font baseline
        int y = 0;
        if ((align & (TA_TOP|TA_BASELINE|TA_BOTTOM)) == TA_TOP)
        {
          TEXTMETRIC linkMetrics;
          ::GetTextMetrics(dc,&linkMetrics);
          y = linkMetrics.tmAscent - metrics.tmAscent;
        }
        if (y != 0)
          ShiftYPos(dc,-y);

        // Output the text in the linked font
        ::ExtTextOutW(dc,0,0,0,&rect,str,theCount,NULL);

        // Put back the font baseline adjustment
        if (y != 0)
          ShiftYPos(dc,y);

        ::SelectObject(dc,font);
        m_fl->ReleaseFont(linkFont);
      }
      else
      {
        // Just use the current font
        ::ExtTextOutW(dc,0,0,0,&rect,str,theCount,NULL);
      }
    }
    str += theCount;
    count -= theCount;
  }

leave:
  // If there is any text left to be output, use the current font
  if (count > 0)
    ::ExtTextOutW(dc,0,0,0,&rect,str,count,NULL);

  // Put back the alignment and position
  if (!(align & TA_UPDATECP))
  {
    ::SetTextAlign(dc,align);
    ::MoveToEx(dc,pos.x,pos.y,NULL);
  }
}

CSize TextOutput::GetTextExtent(HDC dc, LPCWSTR str, UINT count)
{
  CSize result(0,0);

  // If no font linking, just use the current font
  if (m_fl == NULL)
    goto leave;

  // Get the code pages of the current font, which will be used if possible
  HFONT font = (HFONT)::GetCurrentObject(dc,OBJ_FONT);
  DWORD fontCodePages = 0;
  HRESULT hr = m_fl->GetFontCodePages(dc,font,&fontCodePages);
  if (FAILED(hr))
    goto leave;

  while (count > 0)
  {
    // Get the next range that can be output in a font
    DWORD theCodePages = 0;
    long theCount = 0;
    hr = m_fl->GetStrCodePages(str,count,fontCodePages,&theCodePages,&theCount);
    if (FAILED(hr))
      goto leave;

    if (theCodePages & fontCodePages)
    {
      // Use the current font, as it has a suitable code page
      AddTextExtent(dc,str,theCount,result);
    }
    else
    {
      HFONT linkFont = 0;
      if (theCodePages == 0)
      {
        // No code page given, so try to map a single character to a font
        theCount = 1;
        hr = m_fl->MapFont(dc,0,*str,&linkFont);
      }
      else
      {
        // Get a font for the given code pages
        hr = m_fl->MapFont(dc,theCodePages,0,&linkFont);
      }

      if (SUCCEEDED(hr))
      {
        // Got a suitable font, so use it
        ::SelectObject(dc,linkFont);
        AddTextExtent(dc,str,theCount,result);
        ::SelectObject(dc,font);
        m_fl->ReleaseFont(linkFont);
      }
      else
      {
        // Just use the current font
        AddTextExtent(dc,str,theCount,result);
      }
    }
    str += theCount;
    count -= theCount;
  }

leave:
  // If there is any text left to be output, use the current font
  if (count > 0)
    AddTextExtent(dc,str,count,result);
  return result;
}

bool TextOutput::GetCharABCWidth(HDC dc, WCHAR c, ABCFLOAT& abc)
{
  // If GetCharABCWidthsFloatW() is not present, just fail
  if (m_getCharABCWidthsFloatW == NULL)
    return false;

  // If no font linking, just check the current font
  if (m_fl == NULL)
    goto leave;

  // Get the code pages of the current font
  HFONT font = (HFONT)::GetCurrentObject(dc,OBJ_FONT);
  DWORD fontCodePages = 0;
  HRESULT hr = m_fl->GetFontCodePages(dc,font,&fontCodePages);
  if (FAILED(hr))
    goto leave;

  // See if the character is in a code page
  DWORD theCodePages = 0;
  long theCount = 0;
  hr = m_fl->GetStrCodePages(&c,1,fontCodePages,&theCodePages,&theCount);
  if (FAILED(hr))
    goto leave;

  if (theCodePages & fontCodePages)
  {
    // Character is available in the current font
    return ((*m_getCharABCWidthsFloatW)(dc,c,c,&abc) != 0);
  }
  else
  {
    HFONT linkFont = 0;
    if (theCodePages == 0)
    {
      // Try to map the character to a font
      hr = m_fl->MapFont(dc,0,c,&linkFont);
    }
    else
    {
      // Get a font for the given code pages
      hr = m_fl->MapFont(dc,theCodePages,0,&linkFont);
    }

    if (SUCCEEDED(hr))
    {
      // Got a suitable font, so use it
      ::SelectObject(dc,linkFont);
      bool gotABC = ((*m_getCharABCWidthsFloatW)(dc,c,c,&abc) != 0);
      ::SelectObject(dc,font);
      m_fl->ReleaseFont(linkFont);
      return gotABC;
    }
    else
      return ((*m_getCharABCWidthsFloatW)(dc,c,c,&abc) != 0);
  }

leave:
  return ((*m_getCharABCWidthsFloatW)(dc,c,c,&abc) != 0);
}

bool TextOutput::CanOutput(HDC dc, WCHAR c)
{
  // If no font linking, just check the current font
  if (m_fl == NULL)
    goto leave;

  // Get the code pages of the current font
  HFONT font = (HFONT)::GetCurrentObject(dc,OBJ_FONT);
  DWORD fontCodePages = 0;
  HRESULT hr = m_fl->GetFontCodePages(dc,font,&fontCodePages);
  if (FAILED(hr))
    goto leave;

  // See if the character is in a code page
  DWORD theCodePages = 0;
  long theCount = 0;
  hr = m_fl->GetStrCodePages(&c,1,fontCodePages,&theCodePages,&theCount);
  if (FAILED(hr))
    goto leave;

  if (theCodePages & fontCodePages)
  {
    // Character is available in the current font
    return true;
  }
  else
  {
    HFONT linkFont = 0;
    if (theCodePages == 0)
    {
      // Try to map the character to a font
      hr = m_fl->MapFont(dc,0,c,&linkFont);
    }
    else
    {
      // Get a font for the given code pages
      hr = m_fl->MapFont(dc,theCodePages,0,&linkFont);
    }

    if (SUCCEEDED(hr))
    {
      // Character exists in some font
      m_fl->ReleaseFont(linkFont);
      return true;
    }
  }

leave:
  // Try calling the GetGlyphIndicesW() function, if it exists. If calling it fails,
  // just assume that the character can be output and hope for the best.
  if (m_getGlyphIndicesW == NULL)
    return true;
  WORD indices[1] = { 0xFFFF };
  if ((*m_getGlyphIndicesW)(dc,&c,1,indices,GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR)
    return true;

  // Return if a glyph has been found
  return (indices[0] != 0xFFFF);
}

void TextOutput::Reset(void)
{
  // Clear the cache of MLang generated fonts
  if (m_fl != NULL)
    m_fl->ResetFontMapping();
}
