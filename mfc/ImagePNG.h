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

  void Clear(void);
  void SetBackground(COLORREF colour);
  bool LoadResource(UINT resId);
  bool LoadFile(const char* name);
  void Scale(const ImagePNG& image, const CSize& size);

private:
  bool m_back;
  COLORREF m_backColour;

  BYTE* m_pixels;
  CSize m_size;
  double m_aspect;
};
