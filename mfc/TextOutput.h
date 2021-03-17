#pragma once

#include <mlang.h>

#include <map>
#include <string>

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
  bool CanOutput(HDC dc, UINT32 c);

  // Reset the cache of linked fonts
  void Reset(void);

private:
  void AddTextExtent(HDC dc, LPCWSTR str, UINT count, CSize& result);
  void ShiftYPos(HDC dc, int y);

  bool MapFontHigh(HDC dc, UINT32 c, HFONT& font);

  CComPtr<IMLangFontLink2> m_fl;
  std::map<std::pair<std::string,int>,HFONT> m_cache;
};
