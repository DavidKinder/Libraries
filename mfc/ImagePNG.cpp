#include "stdafx.h"
#include "ImagePNG.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

ImagePNG::ImagePNG() : m_back(false), m_backColour(0), m_pixels(NULL), m_aspect(1.0)
{
}

ImagePNG::~ImagePNG()
{
  Clear();
}

BYTE* ImagePNG::Pixels(void) const
{
  return m_pixels;
}

const CSize& ImagePNG::Size(void) const
{
  return m_size;
}

double ImagePNG::AspectRatio(void) const
{
  return m_aspect;
}

void ImagePNG::Draw(CDC* dc, const CPoint& pos) const
{
  ASSERT(m_pixels);

  BITMAPINFOHEADER bitmapInfo = { 0 };
  bitmapInfo.biSize = sizeof bitmapInfo;
  bitmapInfo.biWidth = m_size.cx;
  bitmapInfo.biHeight = m_size.cy*-1;
  bitmapInfo.biPlanes = 1;
  bitmapInfo.biBitCount = 32;
  bitmapInfo.biCompression = BI_RGB;
  ::StretchDIBits(dc->GetSafeHdc(),pos.x,pos.y,m_size.cx,m_size.cy,
    0,0,m_size.cx,m_size.cy,m_pixels,
    (LPBITMAPINFO)&bitmapInfo,DIB_RGB_COLORS,SRCCOPY);
}

void ImagePNG::Clear(void)
{
  delete[] m_pixels;
  m_pixels = NULL;
  m_size = CSize(0,0);
  m_aspect = 1.0;
}

void ImagePNG::SetBackground(COLORREF colour)
{
  m_back = true;
  m_backColour = colour;
}

#include <png.h>
#pragma warning(disable : 4611)

namespace
{
  struct PngDataIO
  {
    BYTE* data;
    ULONG offset;

    static void Read(png_structp png_ptr, png_bytep data, png_size_t length)
    {
      PngDataIO* dataIO = (PngDataIO*)png_get_io_ptr(png_ptr);
      memcpy(data,dataIO->data+dataIO->offset,length);
      dataIO->offset += length;
    }
  };
} // unnamed namespace

bool ImagePNG::LoadResource(UINT resId)
{
  Clear();

  HRSRC res = ::FindResource(NULL,MAKEINTRESOURCE(resId),"PNG");
  if (!res)
    return false;
  HGLOBAL resData = ::LoadResource(NULL,res);
  if (!resData)
    return false;
  BYTE* pngData = (BYTE*)::LockResource(resData);
  if (!pngData)
    return false;

  if (!png_check_sig(pngData,8))
    return false;
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if (!png_ptr)
    return false;
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_read_struct(&png_ptr,NULL,NULL);
    return false;
  }
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    png_destroy_read_struct(&png_ptr,&info_ptr,(png_infopp)NULL);
    return false;
  }
  png_bytep* pixelRows = NULL;
  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
    delete[] pixelRows;
    Clear();
    return false;
  }

  PngDataIO data;
  data.data = pngData;
  data.offset = 8;
  png_set_read_fn(png_ptr,&data,PngDataIO::Read);
  png_set_sig_bytes(png_ptr,8);
  png_read_info(png_ptr,info_ptr);
  m_size.cx = png_get_image_width(png_ptr,info_ptr);
  m_size.cy = png_get_image_height(png_ptr,info_ptr);
  m_aspect = png_get_pixel_aspect_ratio(png_ptr,info_ptr);
  int bit_depth = png_get_bit_depth(png_ptr,info_ptr);
  int color_type = png_get_color_type(png_ptr,info_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8)
    png_set_palette_to_rgb(png_ptr);
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png_ptr);
  if (png_get_valid(png_ptr,info_ptr,PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png_ptr);
  if (bit_depth == 16)
    png_set_strip_16(png_ptr);
  if (bit_depth < 8)
    png_set_packing(png_ptr);
  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  png_set_bgr(png_ptr);
  png_set_filler(png_ptr,0,PNG_FILLER_AFTER);

  if (m_back)
  {
    png_color_16 backColour = { 0 };
    backColour.red = GetRValue(m_backColour);
    backColour.green = GetGValue(m_backColour);
    backColour.blue = GetBValue(m_backColour);
    png_set_background(png_ptr,&backColour,PNG_BACKGROUND_GAMMA_SCREEN,0,1.0);
  }

  m_pixels = new BYTE[m_size.cx*m_size.cy*4];
  pixelRows = new png_bytep[m_size.cy];
  for (int i = 0; i < (int)m_size.cy; i++)
    pixelRows[i] = m_pixels+(m_size.cx*i*4);
  png_read_image(png_ptr,pixelRows);
  png_read_end(png_ptr,end_info);
  png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
  delete[] pixelRows;
  return true;
}

bool ImagePNG::LoadFile(const char* name)
{
  Clear();

  FILE* fp = fopen(name,"rb");
  if (!fp)
    return false;

  unsigned char header[8];
  fread(header,1,8,fp);
  if (!png_check_sig(header,8))
  {
    fclose(fp);
    return false;
  }

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if (!png_ptr)
  {
    fclose(fp);
    return false;
  }
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_read_struct(&png_ptr,NULL,NULL);
    fclose(fp);
    return false;
  }
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    png_destroy_read_struct(&png_ptr,&info_ptr,(png_infopp)NULL);
    fclose(fp);
    return false;
  }
  png_bytep* pixelRows = NULL;
  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
    delete[] pixelRows;
    fclose(fp);
    Clear();
    return false;
  }

  png_init_io(png_ptr,fp);
  png_set_sig_bytes(png_ptr,8);
  png_read_info(png_ptr,info_ptr);
  m_size.cx = png_get_image_width(png_ptr,info_ptr);
  m_size.cy = png_get_image_height(png_ptr,info_ptr);
  m_aspect = png_get_pixel_aspect_ratio(png_ptr,info_ptr);
  int bit_depth = png_get_bit_depth(png_ptr,info_ptr);
  int color_type = png_get_color_type(png_ptr,info_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8)
    png_set_palette_to_rgb(png_ptr);
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png_ptr);
  if (png_get_valid(png_ptr,info_ptr,PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png_ptr);
  if (bit_depth == 16)
    png_set_strip_16(png_ptr);
  if (bit_depth < 8)
    png_set_packing(png_ptr);
  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  png_set_bgr(png_ptr);
  png_set_filler(png_ptr,0,PNG_FILLER_AFTER);

  if (m_back)
  {
    png_color_16 backColour = { 0 };
    backColour.red = GetRValue(m_backColour);
    backColour.green = GetGValue(m_backColour);
    backColour.blue = GetBValue(m_backColour);
    png_set_background(png_ptr,&backColour,PNG_BACKGROUND_GAMMA_SCREEN,0,1.0);
  }

  m_pixels = new BYTE[m_size.cx*m_size.cy*4];
  pixelRows = new png_bytep[m_size.cy];
  for (int i = 0; i < (int)m_size.cy; i++)
    pixelRows[i] = m_pixels+(m_size.cx*i*4);
  png_read_image(png_ptr,pixelRows);
  png_read_end(png_ptr,end_info);
  png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
  delete[] pixelRows;
  fclose(fp);
  return true;
}

#if defined(_WIN64) && !defined(INCLUDE_2PASS_SCALE)
#define INCLUDE_2PASS_SCALE
#endif

#ifdef INCLUDE_2PASS_SCALE
#include <memory>
#include "2PassScale.h"
#else
extern "C" __declspec(dllimport) void ScaleGfx(COLORREF*, UINT, UINT, COLORREF*, UINT, UINT);
#endif

void ImagePNG::Scale(const ImagePNG& image, const CSize& size)
{
  ASSERT(image.m_pixels);

  Clear();
  m_pixels = new BYTE[size.cx*size.cy*4];
  m_size = size;

#ifdef INCLUDE_2PASS_SCALE
  TwoPassScale<BilinearFilter> scaler;
  scaler.Scale(
    (COLORREF*)image.m_pixels,image.m_size.cx,image.m_size.cy,
    (COLORREF*)m_pixels,size.cx,size.cy);
#else
  ScaleGfx(
    (COLORREF*)image.m_pixels,image.m_size.cx,image.m_size.cy,
    (COLORREF*)m_pixels,size.cx,size.cy);
#endif
}
