#include "stdafx.h"
#include "TextOutput.h"

// From wingdi.h
#ifndef GGI_MARK_NONEXISTING_GLYPHS
#define GGI_MARK_NONEXISTING_GLYPHS 1
#endif

// From winnls.h
#ifndef HIGH_SURROGATE_START
#define HIGH_SURROGATE_START 0xd800
#endif
#ifndef HIGH_SURROGATE_END
#define HIGH_SURROGATE_END 0xdbff
#endif

#include <set>

TextOutput::TextOutput()
{
  // Open the GDI32 DLL and get function pointers from it
  m_gdi = ::LoadLibrary("gdi32.dll");
  ASSERT(m_gdi != 0);
  m_getGlyphIndicesW = (GetGlyphIndicesWPtr)::GetProcAddress(m_gdi,"GetGlyphIndicesW");
  m_getCharABCWidthsFloatW = (GetCharABCWidthsFloatWPtr)::GetProcAddress(m_gdi,"GetCharABCWidthsFloatW");

  // Create the MLang COM object
  if (FAILED(m_fl.CoCreateInstance(CLSID_CMultiLanguage)))
    TRACE("Failed to create MLang object\n");
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
      bool linkRelease = true;
      if (theCodePages == 0)
      {
        // No code page given, so try to map a single character to a font
        if ((*str >= HIGH_SURROGATE_START) && (*str <= HIGH_SURROGATE_END))
        {
          theCount = 2;
          UINT32 c = (str[0] << 10) + str[1] + (0x10000 - (0xD800 << 10) - 0xDC00);
          hr = MapFontHigh(dc,c,linkFont) ? S_OK : E_FAIL;
          linkRelease = false;
        }
        else
        {
          theCount = 1;
          hr = m_fl->MapFont(dc,0,*str,&linkFont);
        }
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
        int baseY = 0;
        if ((align & (TA_TOP|TA_BASELINE|TA_BOTTOM)) == TA_TOP)
        {
          TEXTMETRIC linkMetrics;
          ::GetTextMetrics(dc,&linkMetrics);
          baseY = linkMetrics.tmAscent - metrics.tmAscent;
        }
        if (baseY != 0)
          ShiftYPos(dc,-baseY);

        // Output the text in the linked font
        ::TextOutW(dc,0,0,str,theCount);

        // Put back the font baseline adjustment
        if (baseY != 0)
          ShiftYPos(dc,baseY);

        ::SelectObject(dc,font);
        if (linkRelease)
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
      bool linkRelease = true;
      if (theCodePages == 0)
      {
        // No code page given, so try to map a single character to a font
        if ((*str >= HIGH_SURROGATE_START) && (*str <= HIGH_SURROGATE_END))
        {
          theCount = 2;
          UINT32 c = (str[0] << 10) + str[1] + (0x10000 - (0xD800 << 10) - 0xDC00);
          hr = MapFontHigh(dc,c,linkFont) ? S_OK : E_FAIL;
          linkRelease = false;
        }
        else
        {
          theCount = 1;
          hr = m_fl->MapFont(dc,0,*str,&linkFont);
        }
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
        int baseY = 0;
        if ((align & (TA_TOP|TA_BASELINE|TA_BOTTOM)) == TA_TOP)
        {
          TEXTMETRIC linkMetrics;
          ::GetTextMetrics(dc,&linkMetrics);
          baseY = linkMetrics.tmAscent - metrics.tmAscent;
        }
        if (baseY != 0)
          ShiftYPos(dc,-baseY);

        // Output the text in the linked font
        ::ExtTextOutW(dc,0,0,0,&rect,str,theCount,NULL);

        // Put back the font baseline adjustment
        if (baseY != 0)
          ShiftYPos(dc,baseY);

        ::SelectObject(dc,font);
        if (linkRelease)
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
      bool linkRelease = true;
      if (theCodePages == 0)
      {
        // No code page given, so try to map a single character to a font
        if ((*str >= HIGH_SURROGATE_START) && (*str <= HIGH_SURROGATE_END))
        {
          theCount = 2;
          UINT32 c = (str[0] << 10) + str[1] + (0x10000 - (0xD800 << 10) - 0xDC00);
          hr = MapFontHigh(dc,c,linkFont) ? S_OK : E_FAIL;
          linkRelease = false;
        }
        else
        {
          theCount = 1;
          hr = m_fl->MapFont(dc,0,*str,&linkFont);
        }
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
        if (linkRelease)
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
    bool linkRelease = true;
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
      if (linkRelease)
        m_fl->ReleaseFont(linkFont);
      return gotABC;
    }
    else
      return ((*m_getCharABCWidthsFloatW)(dc,c,c,&abc) != 0);
  }

leave:
  return ((*m_getCharABCWidthsFloatW)(dc,c,c,&abc) != 0);
}

bool TextOutput::CanOutput(HDC dc, UINT32 c)
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
  WCHAR wc = 0;
  if (c < 0x10000)
  {
    wc = (WCHAR)c;
    hr = m_fl->GetStrCodePages(&wc,1,fontCodePages,&theCodePages,&theCount);
    if (FAILED(hr))
      goto leave;
  }

  if (theCodePages & fontCodePages)
  {
    // Character is available in the current font
    return true;
  }
  else
  {
    HFONT linkFont = 0;
    bool linkRelease = true;
    if (theCodePages == 0)
    {
      // Try to map the character to a font
      if (c >= 0x10000)
      {
        hr = MapFontHigh(dc,c,linkFont) ? S_OK : E_FAIL;
        linkRelease = false;
      }
      else
        hr = m_fl->MapFont(dc,0,wc,&linkFont);
    }
    else
    {
      // Get a font for the given code pages
      hr = m_fl->MapFont(dc,theCodePages,0,&linkFont);
    }

    if (SUCCEEDED(hr))
    {
      // Character exists in some font
      if (linkRelease)
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
  if ((*m_getGlyphIndicesW)(dc,&wc,1,indices,GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR)
    return true;

  // Return if a glyph has been found
  return (indices[0] != 0xFFFF);
}

void TextOutput::Reset(void)
{
  // Clear the cache of MLang generated fonts
  if (m_fl != NULL)
    m_fl->ResetFontMapping();

  // Clear the cache of directly created fonts
  for (std::map<std::pair<std::string,int>,HFONT>::iterator it = m_cache.begin(); it != m_cache.end(); ++it)
    ::DeleteObject(it->second);
  m_cache.clear();
}

void TextOutput::AddTextExtent(HDC dc, LPCWSTR str, UINT count, CSize& result)
{
  SIZE sz = { 0,0 };
  if (::GetTextExtentPoint32W(dc,str,count,&sz))
  {
    result.cx += sz.cx;
    if (sz.cy > result.cy)
      result.cy = sz.cy;
  }
}

void TextOutput::ShiftYPos(HDC dc, int y)
{
  POINT pos;
  ::GetCurrentPositionEx(dc,&pos);
  ::MoveToEx(dc,pos.x,pos.y+y,NULL);
}

namespace {

enum HighFont
{
  High_Emoji = 0,
  High_Math,
  High_Symbol,
  Num_High_Fonts,
  High_Invalid
};

struct HighRange
{
  UINT32 start, nextStart;
  HighFont font;

  HighRange(UINT32 s, UINT32 ns, HighFont f) : start(s), nextStart(ns), font(f)
  {
  }
};

int CALLBACK FindFontsProc(ENUMLOGFONTEX* lf, NEWTEXTMETRICEX* ,DWORD fontType, LPARAM param)
{
  if (fontType & TRUETYPE_FONTTYPE)
  {
    std::set<std::string>* names = (std::set<std::string>*)param;
    names->insert((char*)(lf->elfFullName));
  }
  return 1;
}

std::string findFontName(const std::set<std::string>& allFontNames, const char** possibleNames)
{
  for (const char** ptr = possibleNames; *ptr != NULL; ptr++)
  {
    if (allFontNames.count(*ptr) > 0)
      return *ptr;
  }
  return "";
}
} // unnamed namespace

bool TextOutput::MapFontHigh(HDC dc, UINT32 c, HFONT& font)
{
  // Find the Unicode block for the character
  static HighRange unicodeBlocks[] = 
  {
    HighRange(0x1F100,0x1F200,High_Emoji),  // UBLOCK_ENCLOSED_ALPHANUMERIC_SUPPLEMENT
    HighRange(0x1F600,0x1F650,High_Emoji),  // UBLOCK_EMOTICONS

    HighRange(0x10330,0x10350,High_Symbol), // UBLOCK_GOTHIC
    HighRange(0x1F0A0,0x1F100,High_Symbol), // UBLOCK_PLAYING_CARDS
    HighRange(0x1F300,0x1F600,High_Symbol), // UBLOCK_MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS
    HighRange(0x1F680,0x1F700,High_Symbol), // UBLOCK_TRANSPORT_AND_MAP_SYMBOLS
    HighRange(0x1F700,0x1F780,High_Symbol), // UBLOCK_ALCHEMICAL_SYMBOLS

    HighRange(0x1D400,0x1D800,High_Math),   // UBLOCK_MATHEMATICAL_ALPHANUMERIC_SYMBOLS
    HighRange(0x1EE00,0x1EF00,High_Math),   // UBLOCK_ARABIC_MATHEMATICAL_ALPHABETIC_SYMBOLS
    HighRange(0x1F780,0x1F800,High_Math)    // UBLOCK_GEOMETRIC_SHAPES_EXTENDED
  };
  HighFont hf = High_Invalid;
  for (int i = 0; i < sizeof unicodeBlocks / sizeof unicodeBlocks[0]; i++)
  {
    if ((c >= unicodeBlocks[i].start) && (c < unicodeBlocks[i].nextStart))
    {
      hf = unicodeBlocks[i].font;
      break;
    }
  }
  if (hf == High_Invalid)
    return false;

  // Find the appropriate font name
  static std::string highFontNames[Num_High_Fonts];
  static bool fontSearched = false;
  if (!fontSearched)
  {
    std::set<std::string> allFontNames;
    LOGFONT lf = { 0 };
    lf.lfCharSet = DEFAULT_CHARSET;
    ::EnumFontFamiliesEx(dc,&lf,(FONTENUMPROC)FindFontsProc,(LPARAM)&allFontNames,0);

    const char* emojiFontNames[] = { "Segoe UI Emoji", "Segoe UI Symbol", NULL };
    highFontNames[High_Emoji] = findFontName(allFontNames,emojiFontNames);
    const char* mathFontNames[] = { "Cambria Math", "Segoe UI Symbol", "Code2000", NULL };
    highFontNames[High_Math] = findFontName(allFontNames,mathFontNames);
    const char* symbolFontNames[] = { "Segoe UI Symbol", NULL };
    highFontNames[High_Symbol] = findFontName(allFontNames,symbolFontNames);
    fontSearched = true;
  }
  if (highFontNames[hf].empty())
    return false;

  // Create a LOGFONT structure for the required font
  LOGFONT lf = { 0 };
  ::GetObject(::GetCurrentObject(dc,OBJ_FONT),sizeof lf,&lf);
  strcpy(lf.lfFaceName,highFontNames[hf].c_str());

  // Is this font in the cache?
  std::pair<std::string,int> key(highFontNames[hf],abs(lf.lfHeight));
  if (m_cache.count(key) > 0)
  {
    font = m_cache[key];
    return true;
  }

  // Create the required font
  font = ::CreateFontIndirect(&lf);
  if (font != 0)
  {
    m_cache[key] = font;
    return true;
  }
  return false;
}
