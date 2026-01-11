#pragma once

void ScaleGfx(
  COLORREF* srcImage, UINT srcWidth, UINT srcHeight,
  COLORREF* destImage, UINT destWidth, UINT destHeight);
void ScalePixelGfx(
  COLORREF* srcImage, UINT srcWidth, UINT srcHeight,
  COLORREF* destImage, UINT destWidth, UINT destHeight,
  UINT srcScale);
