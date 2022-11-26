#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <ProcGenPlugin/Declarations.h>

namespace xiiProcGenInternal
{
  static Pattern::Point s_BayerPoints[64];

  static xiiHashTable<xiiUInt64, Pattern, xiiHashHelper<xiiUInt64>, xiiStaticAllocatorWrapper> s_Patterns;

  bool FillPatterns()
  {
    // generate Bayer pattern
    const xiiUInt32 M = 3;
    const xiiUInt32 n = 1 << M;

    for (xiiUInt32 y = 0; y < n; ++y)
    {
      for (xiiUInt32 x = 0; x < n; ++x)
      {
        xiiUInt32 v = 0, mask = M - 1, xc = x ^ y, yc = y;
        for (xiiUInt32 bit = 0; bit < 2 * M; --mask)
        {
          v |= ((yc >> mask) & 1) << bit++;
          v |= ((xc >> mask) & 1) << bit++;
        }

        auto& point = s_BayerPoints[y * n + x];
        point.m_Coordinates.Set(x + 0.5f, y + 0.5f);
        point.m_fThreshold = (v + 1.0f) / (n * n);
      }
    }

    Pattern bayerPattern;
    bayerPattern.m_Points = xiiMakeArrayPtr(s_BayerPoints);
    bayerPattern.m_fSize  = (float)n;

    s_Patterns.Insert(xiiTempHashedString("Bayer").GetHash(), bayerPattern);

    return true;
  }

  static bool s_bFillPatternsDummy = FillPatterns();

  Pattern* GetPattern(xiiTempHashedString sName) { return s_Patterns.GetValue(sName.GetHash()); }
} // namespace xiiProcGenInternal
