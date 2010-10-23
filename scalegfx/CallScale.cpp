#include <windows.h>

namespace {
#include "2PassScale.h"
}

void FUNCTION(COLORREF* srcImage, UINT srcWidth, UINT srcHeight, COLORREF* destImage, UINT destWidth, UINT destHeight)
{
  TwoPassScale<BilinearFilter> scaler;
  scaler.Scale(srcImage,srcWidth,srcHeight,destImage,destWidth,destHeight);
}

