#pragma once

class ImagePNG
{
public:
  ImagePNG();
  ~ImagePNG();

  BYTE* Pixels(void) const;
  const CSize& Size(void) const;
  double AspectRatio(void) const;
  void Draw(CDC* dc, const CPoint& pos) const;
  HBITMAP CopyBitmap(CWnd* wnd) const;

  void Clear(void);
  void Copy(const ImagePNG& from);
  void Fill(COLORREF colour);
  void Blend(COLORREF colour);
  void SetBackground(COLORREF colour);
  bool LoadResource(UINT resId);
  bool LoadFile(const char* name);
  void Scale(const ImagePNG& image, const CSize& size);

private:
  void FillBitmapInfo(BITMAPINFOHEADER& info) const;

  bool m_back;
  COLORREF m_backColour;

  BYTE* m_pixels;
  CSize m_size;
  double m_aspect;
};
