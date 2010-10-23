#include <windows.h>

#ifdef TEST
#include <cstdio>
#endif

void ScaleNoSSE(COLORREF*, UINT, UINT, COLORREF*, UINT, UINT);
void ScaleSSE(COLORREF*, UINT, UINT, COLORREF*, UINT, UINT);

enum StatusSSE
{
  NotChecked = 0,
  NotPresent,
  Present
};
StatusSSE sse = NotChecked;

extern "C" __declspec(dllexport) void ScaleGfx(
  COLORREF* srcImage, UINT srcWidth, UINT srcHeight, COLORREF* destImage, UINT destWidth, UINT destHeight)
{
  GdiFlush();

  if (sse == NotChecked)
  {
#ifdef TEST
    MessageBox(0,"SSE status has not been checked","ScaleGfx",MB_OK);
#endif

    sse = NotPresent;

    HINSTANCE kernel = LoadLibrary("kernel32.dll");
    if (kernel != 0)
    {
      typedef BOOL(__stdcall *ISPROCESSORFEATUREPRESENT)(DWORD);

      ISPROCESSORFEATUREPRESENT isProcessorFeaturePresent =
        (ISPROCESSORFEATUREPRESENT)::GetProcAddress(kernel,"IsProcessorFeaturePresent");
      if (isProcessorFeaturePresent)
      {
        if ((*isProcessorFeaturePresent)(PF_XMMI_INSTRUCTIONS_AVAILABLE) != 0)
          sse = Present;
      }
      FreeLibrary(kernel);
    }
  }

#ifdef TEST
  char msg[256];
  sprintf(msg,"SSE status is %d",(int)sse);
  MessageBox(0,msg,"ScaleGfx",MB_OK);
#endif

  if (sse == Present)
  {
#ifdef TEST
    MessageBox(0,"Calling SSE scaling","ScaleGfx",MB_OK);
#endif
    return ScaleSSE(srcImage,srcWidth,srcHeight,destImage,destWidth,destHeight);
  }

#ifdef TEST
  MessageBox(0,"Calling non-SSE scaling","ScaleGfx",MB_OK);
#endif
  return ScaleNoSSE(srcImage,srcWidth,srcHeight,destImage,destWidth,destHeight);
}
