// Taken from an article by Eran Yariv at http://www.codeproject.com/

#ifndef TWO_PASS_SCALE_H_
#define TWO_PASS_SCALE_H_

#include <math.h>

#define FILTER_PI  (3.1415926535897932384626433832795f)
#define FILTER_2PI (2.0f * 3.1415926535897932384626433832795f)
#define FILTER_4PI (4.0f * 3.1415926535897932384626433832795f)

class GenericFilter
{
public:
  GenericFilter(float width) : m_width(width)
  {
  }
  float m_width;
};

class BoxFilter : public GenericFilter
{
public:
  BoxFilter(float width = 0.5f) : GenericFilter(width)
  {
  }
  float Filter(float val)
  {
    return (fabs(val) <= m_width ? 1.0f : 0.0f);
  }
};

class BilinearFilter : public GenericFilter
{
public:
  BilinearFilter(float width = 1.0f) : GenericFilter(width)
  {
  }
  float Filter(float val)
  {
    val = (float)fabs(val);
    return (val < m_width ? m_width - val : 0.0f);
  }
};

class GaussianFilter : public GenericFilter
{
public:
  GaussianFilter(float width = 3.0f) : GenericFilter(width)
  {
  }
  float Filter(float val)
  {
    if (fabs(val) > m_width)
      return 0.0f;
    return (float)(exp(-val * val / 2.0f) / sqrt(FILTER_2PI));
  }
};

class HammingFilter : public GenericFilter
{
public:
  HammingFilter(float width = 0.5f) : GenericFilter(width)
  {
  }
  float Filter(float val)
  {
    if (fabs(val) > m_width)
      return 0.0f;
    float window = (float)(0.54f + 0.46f * cos(FILTER_2PI * val));
    float sinc = (val == 0.0f) ? 1.0f : (float)(sin(FILTER_PI * val) / (FILTER_PI * val));
    return window * sinc;
  }
};

class BlackmanFilter : public GenericFilter
{
public:
  BlackmanFilter(float width = 0.5f) : GenericFilter(width)
  {
  }
  float Filter(float val)
  {
    if (fabs(val) > m_width)
      return 0.0f;
    float n = 2.0f * m_width + 1.0f;
    return (float)(0.42f + 0.5f * cos(FILTER_2PI * val / (n - 1.0f)) + 0.08f * cos(FILTER_4PI * val / (n - 1.0f)));
  }
};

// Contirbution information for a single pixel
typedef struct
{
  float* weights;   // Normalized weights of neighbouring pixels
  int left, right;  // Bounds of source pixels window
}
ContributionType;

// Contribution information for an entire line (row or column)
typedef struct
{
  ContributionType *row; // Row (or column) of contribution weights
  UINT lineLen;          // Length of line (no. or rows / cols)
}
LineContribType;

template <class FilterClass>
class TwoPassScale
{
public:
  void Scale(COLORREF* origImage, UINT origWidth, UINT origHeight, COLORREF* dstImage, UINT newWidth, UINT newHeight);

private:
  LineContribType* AllocContributions(UINT lineLen, UINT winSize);
  void FreeContributions(LineContribType* contrib);
  LineContribType* CalcContributions(UINT lineSize, UINT srcSize, float scale);
  inline void AddWeight(COLORREF src, float weight, int& r, int& g, int& b, int& a);
  void ScaleRow(COLORREF* src, UINT srcWidth, COLORREF* res, UINT resWidth, UINT row, LineContribType* contrib);
  void HorizScale(COLORREF* src, UINT srcWidth, UINT srcHeight, COLORREF* dst, UINT resWidth, UINT resHeight);
  void ScaleCol(COLORREF* src, UINT srcWidth,
    COLORREF* res, UINT resWidth, UINT resHeight, UINT col, LineContribType* contrib);
  void VertScale(COLORREF* src, UINT srcWidth, UINT srcHeight, COLORREF* dst, UINT resWidth, UINT resHeight);
};

template<class FilterClass>
LineContribType* TwoPassScale<FilterClass>::AllocContributions(UINT lineLen, UINT winSize)
{
  LineContribType* res = new LineContribType;
  // Init structure header
  res->lineLen = lineLen;
  // Allocate list of contributions
  res->row = new ContributionType[lineLen];
  // Allocate contributions for every pixel
  for (UINT i = 0; i < lineLen; i++)
    res->row[i].weights = new float[winSize];
  return res;
}

template<class FilterClass>
void TwoPassScale<FilterClass>::FreeContributions(LineContribType* contrib)
{
  // Free contribs for every pixel
  for (UINT i = 0; i < contrib->lineLen; i++)
    delete[] contrib->row[i].weights;
  delete[] contrib->row; // Free list of pixels contribs
  delete contrib;        // Free contributions header
}

template<class FilterClass>
LineContribType* TwoPassScale<FilterClass>::CalcContributions(UINT lineSize, UINT srcSize, float scale)
{
  FilterClass filter;

  float width;
  float fscale = 1.0f;
  float filterWidth = filter.m_width;

  if (scale < 1.0f)
  {
    // Minification
    width = filterWidth / scale;
    fscale = scale;
  }
  else
  {
    // Magnification
    width = filterWidth;
  }

  // Window size is the number of sampled pixels
  int winSize = 2 * (int)ceil(width) + 1;

  // Allocate a new line contributions strucutre
  LineContribType* res = AllocContributions(lineSize,winSize);

  for (UINT i = 0; i < lineSize; i++)
  {
    // Scan through line of contributions
    float center = (float)i / scale; // Reverse mapping
    // Find the significant edge points that affect the pixel
    int left = max(0,(int)floor(center - width));
    int right = min((int)ceil(center + width),((int)srcSize) - 1);

    // Cut edge points to fit in filter window in case of spill-off
    if (right - left + 1 > winSize)
    {
      if (left < (((int)srcSize) - 1 / 2))
        left++;
      else
        right--;
    }
    res->row[i].left = left;
    res->row[i].right = right;

    float totalWeight = 0.0f; // Zero sum of weights
    for (int src = left; src <= right; src++)
    {
      // Calculate weights
      totalWeight += (res->row[i].weights[src-left] =
        fscale * filter.Filter(fscale * (center - (float)src)));
    }
    if (totalWeight > 0.0f)
    {
      // Normalize weight of neighbouring points
      for (int src = left; src <= right; src++)
        res->row[i].weights[src-left] *= (((float)(1U<<22)) / totalWeight);
    }
  }
  return res;
}

template<class FilterClass>
inline void TwoPassScale<FilterClass>::AddWeight(COLORREF src, float weight, int& r, int& g, int& b, int& a)
{
  int w = (int)weight;
  r += (int)(GetRValue(src) * w) >> 22;
  g += (int)(GetGValue(src) * w) >> 22;
  b += (int)(GetBValue(src) * w) >> 22;
  a += (int)((src >> 24) * w) >> 22;
}

template<class FilterClass>
void TwoPassScale<FilterClass>::ScaleRow(COLORREF* src, UINT srcWidth, COLORREF* res, UINT resWidth,
                                         UINT row, LineContribType* contrib)
{
  COLORREF* srcRow = src + (row * srcWidth);
  COLORREF* dstRow = res + (row * resWidth);
  for (UINT x = 0; x < resWidth; x++)
  {
    // Loop through row
    int r = 0; int g = 0; int b = 0; int a = 0;
    int left = contrib->row[x].left;   // Retrieve left boundary
    int right = contrib->row[x].right; // Retrieve right boundary
    int same = 1; COLORREF color = srcRow[left];
    for (int i = left+1; i <= right; i++)
    {
      if (srcRow[i] != color)
      {
        same = 0;
        break;
      }
    }
    if (same)
      dstRow[x] = color;
    else
    {
      for (int i = left; i <= right; i++)
      {
        // Scan between boundaries
        // Accumulate weighted effect of each neighboring pixel
        AddWeight(srcRow[i],contrib->row[x].weights[i-left],r,g,b,a);
      }
      dstRow[x] = RGB(r,g,b) | (a << 24); // Place result in destination pixel
    }
  }
}

template<class FilterClass>
void TwoPassScale<FilterClass>::HorizScale(COLORREF* src, UINT srcWidth, UINT srcHeight, COLORREF* dst,
                                           UINT resWidth, UINT resHeight)
{
  if (resWidth == srcWidth)
  {
    // No scaling required, just copy
    memcpy(dst,src,sizeof (COLORREF) * srcHeight * srcWidth);
  }
  // Allocate and calculate the contributions
  LineContribType* contrib = CalcContributions(resWidth,srcWidth,(float)resWidth / (float)srcWidth);
  for (UINT i = 0; i < resHeight; i++)
  {
    // Step through rows
    ScaleRow(src,srcWidth,dst,resWidth,i,contrib);
  }
  FreeContributions(contrib); // Free contributions structure
}

template<class FilterClass>
void TwoPassScale<FilterClass>::ScaleCol(COLORREF* src, UINT srcWidth, COLORREF* res, UINT resWidth, UINT resHeight,
                                         UINT col, LineContribType* contrib)
{
  for (UINT y = 0; y < resHeight; y++)
  {
    // Loop through column
    int r = 0; int g = 0; int b = 0; int a = 0;
    int left = contrib->row[y].left;    // Retrieve left boundary
    int right = contrib->row[y].right;  // Retrieve right boundary
    int same = 1; COLORREF color = src[left * srcWidth + col];
    for (int i = left+1; i <= right; i++)
    {
      if (src[i * srcWidth + col] != color)
      {
        same = 0;
        break;
      }
    }
    if (same)
      res[y * resWidth + col] = color;
    else
    {
      for (int i = left; i <= right; i++)
      {
        // Scan between boundaries
        // Accumulate weighted effect of each neighboring pixel
        AddWeight(src[i * srcWidth + col],contrib->row[y].weights[i-left],r,g,b,a);
      }
      res[y * resWidth + col] = RGB(r,g,b) | (a << 24); // Place result in destination pixel
    }
  }
}

template<class FilterClass>
void TwoPassScale<FilterClass>::VertScale(COLORREF* src, UINT srcWidth, UINT srcHeight, COLORREF* dst,
                                          UINT resWidth, UINT resHeight)
{
  if (srcHeight == resHeight)
  {
    // No scaling required, just copy
    memcpy (dst,src,sizeof (COLORREF) * srcHeight * srcWidth);
  }
  // Allocate and calculate the contributions
  LineContribType* contrib = CalcContributions(resHeight,srcHeight,(float)resHeight / (float)srcHeight);
  for (UINT i = 0; i < resWidth; i++)
  {
    // Step through columns
    ScaleCol(src,srcWidth,dst,resWidth,resHeight,i,contrib);
  }
  FreeContributions(contrib); // Free contributions structure
}

template<class FilterClass>
void TwoPassScale<FilterClass>::Scale(COLORREF* origImage, UINT origWidth, UINT origHeight,
                                      COLORREF* dstImage, UINT newWidth, UINT newHeight)
{
  // Scale source image horizontally into temporary image
  COLORREF* temp = new COLORREF[newWidth * origHeight];
  HorizScale(origImage,origWidth,origHeight,temp,newWidth,origHeight);

  // Scale temporary image vertically into result image
  VertScale(temp,newWidth,origHeight,dstImage,newWidth,newHeight);
  delete[] temp;
}

#endif // TWO_PASS_SCALE_H_
