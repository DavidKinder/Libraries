#pragma once

#include <mlang.h>

class TextOutput
{
public:
  TextOutput();
  ~TextOutput();

  // Output a string
  void TextOut(HDC dc, int x, int y, LPCWSTR str, UINT count);

  // Output a string in a rectangle
  void TextOut(HDC dc, int x, int y, LPCWSTR str, UINT count, const RECT& rect, bool opaque);

  // Get the dimensions of a string
  CSize GetTextExtent(HDC dc, LPCWSTR str, UINT count);

  // Get the floating point ABC widths of a character
  bool GetCharABCWidth(HDC dc, WCHAR c, ABCFLOAT& abc);

  // Determine if a given character can be output
  bool CanOutput(HDC dc, WCHAR c);

  // Reset the cache of linked fonts
  void Reset(void);

private:
  CComPtr<IMLangFontLink2> m_fl;
  HMODULE m_gdi;

  typedef DWORD (WINAPI *GetGlyphIndicesWPtr)(HDC, LPCWSTR, int, LPWORD, DWORD);
  GetGlyphIndicesWPtr m_getGlyphIndicesW;
  typedef BOOL (WINAPI *GetCharABCWidthsFloatWPtr)(HDC, UINT, UINT, LPABCFLOAT);
  GetCharABCWidthsFloatWPtr m_getCharABCWidthsFloatW;
};
