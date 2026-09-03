/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/ColorScheme.h>

xiiColor xiiColorScheme::s_Colors[Count][10] = {
  {
    xiiColorGammaUB(201, 42, 42),   // oc-red-9
    xiiColorGammaUB(224, 49, 49),   // oc-red-8
    xiiColorGammaUB(240, 62, 62),   // oc-red-7
    xiiColorGammaUB(250, 82, 82),   // oc-red-6
    xiiColorGammaUB(255, 107, 107), // oc-red-5
    xiiColorGammaUB(255, 135, 135), // oc-red-4
    xiiColorGammaUB(255, 168, 168), // oc-red-3
    xiiColorGammaUB(255, 201, 201), // oc-red-2
    xiiColorGammaUB(255, 227, 227), // oc-red-1
    xiiColorGammaUB(255, 245, 245), // oc-red-0
  },
  {
    xiiColorGammaUB(166, 30, 77),   // oc-pink-9
    xiiColorGammaUB(194, 37, 92),   // oc-pink-8
    xiiColorGammaUB(214, 51, 108),  // oc-pink-7
    xiiColorGammaUB(230, 73, 128),  // oc-pink-6
    xiiColorGammaUB(240, 101, 149), // oc-pink-5
    xiiColorGammaUB(247, 131, 172), // oc-pink-4
    xiiColorGammaUB(250, 162, 193), // oc-pink-3
    xiiColorGammaUB(252, 194, 215), // oc-pink-2
    xiiColorGammaUB(255, 222, 235), // oc-pink-1
    xiiColorGammaUB(255, 240, 246), // oc-pink-0
  },
  {
    xiiColorGammaUB(134, 46, 156),  // oc-grape-9
    xiiColorGammaUB(156, 54, 181),  // oc-grape-8
    xiiColorGammaUB(174, 62, 201),  // oc-grape-7
    xiiColorGammaUB(190, 75, 219),  // oc-grape-6
    xiiColorGammaUB(204, 93, 232),  // oc-grape-5
    xiiColorGammaUB(218, 119, 242), // oc-grape-4
    xiiColorGammaUB(229, 153, 247), // oc-grape-3
    xiiColorGammaUB(238, 190, 250), // oc-grape-2
    xiiColorGammaUB(243, 217, 250), // oc-grape-1
    xiiColorGammaUB(248, 240, 252), // oc-grape-0
  },
  {
    xiiColorGammaUB(95, 61, 196),   // oc-violet-9
    xiiColorGammaUB(103, 65, 217),  // oc-violet-8
    xiiColorGammaUB(112, 72, 232),  // oc-violet-7
    xiiColorGammaUB(121, 80, 242),  // oc-violet-6
    xiiColorGammaUB(132, 94, 247),  // oc-violet-5
    xiiColorGammaUB(151, 117, 250), // oc-violet-4
    xiiColorGammaUB(177, 151, 252), // oc-violet-3
    xiiColorGammaUB(208, 191, 255), // oc-violet-2
    xiiColorGammaUB(229, 219, 255), // oc-violet-1
    xiiColorGammaUB(243, 240, 255), // oc-violet-0
  },
  {
    xiiColorGammaUB(54, 79, 199),   // oc-indigo-9
    xiiColorGammaUB(59, 91, 219),   // oc-indigo-8
    xiiColorGammaUB(66, 99, 235),   // oc-indigo-7
    xiiColorGammaUB(76, 110, 245),  // oc-indigo-6
    xiiColorGammaUB(92, 124, 250),  // oc-indigo-5
    xiiColorGammaUB(116, 143, 252), // oc-indigo-4
    xiiColorGammaUB(145, 167, 255), // oc-indigo-3
    xiiColorGammaUB(186, 200, 255), // oc-indigo-2
    xiiColorGammaUB(219, 228, 255), // oc-indigo-1
    xiiColorGammaUB(237, 242, 255), // oc-indigo-0
  },
  {
    xiiColorGammaUB(24, 100, 171),  // oc-blue-9
    xiiColorGammaUB(25, 113, 194),  // oc-blue-8
    xiiColorGammaUB(28, 126, 214),  // oc-blue-7
    xiiColorGammaUB(34, 139, 230),  // oc-blue-6
    xiiColorGammaUB(51, 154, 240),  // oc-blue-5
    xiiColorGammaUB(77, 171, 247),  // oc-blue-4
    xiiColorGammaUB(116, 192, 252), // oc-blue-3
    xiiColorGammaUB(165, 216, 255), // oc-blue-2
    xiiColorGammaUB(208, 235, 255), // oc-blue-1
    xiiColorGammaUB(231, 245, 255), // oc-blue-0
  },
  {
    xiiColorGammaUB(11, 114, 133),  // oc-cyan-9
    xiiColorGammaUB(12, 133, 153),  // oc-cyan-8
    xiiColorGammaUB(16, 152, 173),  // oc-cyan-7
    xiiColorGammaUB(21, 170, 191),  // oc-cyan-6
    xiiColorGammaUB(34, 184, 207),  // oc-cyan-5
    xiiColorGammaUB(59, 201, 219),  // oc-cyan-4
    xiiColorGammaUB(102, 217, 232), // oc-cyan-3
    xiiColorGammaUB(153, 233, 242), // oc-cyan-2
    xiiColorGammaUB(197, 246, 250), // oc-cyan-1
    xiiColorGammaUB(227, 250, 252), // oc-cyan-0
  },
  {
    xiiColorGammaUB(8, 127, 91),    // oc-teal-9
    xiiColorGammaUB(9, 146, 104),   // oc-teal-8
    xiiColorGammaUB(12, 166, 120),  // oc-teal-7
    xiiColorGammaUB(18, 184, 134),  // oc-teal-6
    xiiColorGammaUB(32, 201, 151),  // oc-teal-5
    xiiColorGammaUB(56, 217, 169),  // oc-teal-4
    xiiColorGammaUB(99, 230, 190),  // oc-teal-3
    xiiColorGammaUB(150, 242, 215), // oc-teal-2
    xiiColorGammaUB(195, 250, 232), // oc-teal-1
    xiiColorGammaUB(230, 252, 245), // oc-teal-0
  },
  {
    xiiColorGammaUB(43, 138, 62),   // oc-green-9
    xiiColorGammaUB(47, 158, 68),   // oc-green-8
    xiiColorGammaUB(55, 178, 77),   // oc-green-7
    xiiColorGammaUB(64, 192, 87),   // oc-green-6
    xiiColorGammaUB(81, 207, 102),  // oc-green-5
    xiiColorGammaUB(105, 219, 124), // oc-green-4
    xiiColorGammaUB(140, 233, 154), // oc-green-3
    xiiColorGammaUB(178, 242, 187), // oc-green-2
    xiiColorGammaUB(211, 249, 216), // oc-green-1
    xiiColorGammaUB(235, 251, 238), // oc-green-0
  },
  {
    xiiColorGammaUB(92, 148, 13),   // oc-lime-9
    xiiColorGammaUB(102, 168, 15),  // oc-lime-8
    xiiColorGammaUB(116, 184, 22),  // oc-lime-7
    xiiColorGammaUB(130, 201, 30),  // oc-lime-6
    xiiColorGammaUB(148, 216, 45),  // oc-lime-5
    xiiColorGammaUB(169, 227, 75),  // oc-lime-4
    xiiColorGammaUB(192, 235, 117), // oc-lime-3
    xiiColorGammaUB(216, 245, 162), // oc-lime-2
    xiiColorGammaUB(233, 250, 200), // oc-lime-1
    xiiColorGammaUB(244, 252, 227), // oc-lime-0
  },
  {
    xiiColorGammaUB(230, 119, 0),   // oc-yellow-9
    xiiColorGammaUB(240, 140, 0),   // oc-yellow-8
    xiiColorGammaUB(245, 159, 0),   // oc-yellow-7
    xiiColorGammaUB(250, 176, 5),   // oc-yellow-6
    xiiColorGammaUB(252, 196, 25),  // oc-yellow-5
    xiiColorGammaUB(255, 212, 59),  // oc-yellow-4
    xiiColorGammaUB(255, 224, 102), // oc-yellow-3
    xiiColorGammaUB(255, 236, 153), // oc-yellow-2
    xiiColorGammaUB(255, 243, 191), // oc-yellow-1
    xiiColorGammaUB(255, 249, 219), // oc-yellow-0
  },
  {
    xiiColorGammaUB(217, 72, 15),   // oc-orange-9
    xiiColorGammaUB(232, 89, 12),   // oc-orange-8
    xiiColorGammaUB(247, 103, 7),   // oc-orange-7
    xiiColorGammaUB(253, 126, 20),  // oc-orange-6
    xiiColorGammaUB(255, 146, 43),  // oc-orange-5
    xiiColorGammaUB(255, 169, 77),  // oc-orange-4
    xiiColorGammaUB(255, 192, 120), // oc-orange-3
    xiiColorGammaUB(255, 216, 168), // oc-orange-2
    xiiColorGammaUB(255, 232, 204), // oc-orange-1
    xiiColorGammaUB(255, 244, 230), // oc-orange-0
  },
  {
    xiiColorGammaUB(33, 37, 41),    // oc-gray-9
    xiiColorGammaUB(52, 58, 64),    // oc-gray-8
    xiiColorGammaUB(73, 80, 87),    // oc-gray-7
    xiiColorGammaUB(134, 142, 150), // oc-gray-6
    xiiColorGammaUB(173, 181, 189), // oc-gray-5
    xiiColorGammaUB(206, 212, 218), // oc-gray-4
    xiiColorGammaUB(222, 226, 230), // oc-gray-3
    xiiColorGammaUB(233, 236, 239), // oc-gray-2
    xiiColorGammaUB(241, 243, 245), // oc-gray-1
    xiiColorGammaUB(248, 249, 250), // oc-gray-0
  },
};

// We could use a lower brightness here for our dark UI but the colors looks much nicer at higher brightness so we just apply a scale factor instead.
static constexpr xiiUInt8 DarkUIBrightness                      = 3;
static constexpr xiiUInt8 DarkUIGrayBrightness                  = 4; // gray is too dark at UIBrightness
static constexpr float    DarkUISaturation                      = 0.95f;
static constexpr xiiColor DarkUIFactor                          = xiiColor(0.5f, 0.5f, 0.5f, 1.0f);
xiiColor                  xiiColorScheme::s_DarkUIColors[Count] = {
  GetColor(xiiColorScheme::Red, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(xiiColorScheme::Pink, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(xiiColorScheme::Grape, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(xiiColorScheme::Violet, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(xiiColorScheme::Indigo, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(xiiColorScheme::Blue, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(xiiColorScheme::Cyan, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(xiiColorScheme::Teal, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(xiiColorScheme::Green, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(xiiColorScheme::Lime, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(xiiColorScheme::Yellow, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(xiiColorScheme::Orange, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(xiiColorScheme::Gray, DarkUIGrayBrightness, DarkUISaturation) * DarkUIFactor,
};

static constexpr xiiUInt8 LightUIBrightness                      = 4;
static constexpr xiiUInt8 LightUIGrayBrightness                  = 5; // gray is too dark at UIBrightness
static constexpr float    LightUISaturation                      = 1.0f;
xiiColor                  xiiColorScheme::s_LightUIColors[Count] = {
  GetColor(xiiColorScheme::Red, LightUIBrightness, LightUISaturation),
  GetColor(xiiColorScheme::Pink, LightUIBrightness, LightUISaturation),
  GetColor(xiiColorScheme::Grape, LightUIBrightness, LightUISaturation),
  GetColor(xiiColorScheme::Violet, LightUIBrightness, LightUISaturation),
  GetColor(xiiColorScheme::Indigo, LightUIBrightness, LightUISaturation),
  GetColor(xiiColorScheme::Blue, LightUIBrightness, LightUISaturation),
  GetColor(xiiColorScheme::Cyan, LightUIBrightness, LightUISaturation),
  GetColor(xiiColorScheme::Teal, LightUIBrightness, LightUISaturation),
  GetColor(xiiColorScheme::Green, LightUIBrightness, LightUISaturation),
  GetColor(xiiColorScheme::Lime, LightUIBrightness, LightUISaturation),
  GetColor(xiiColorScheme::Yellow, LightUIBrightness, LightUISaturation),
  GetColor(xiiColorScheme::Orange, LightUIBrightness, LightUISaturation),
  GetColor(xiiColorScheme::Gray, LightUIGrayBrightness, LightUISaturation),
};

// static
xiiColor xiiColorScheme::GetColor(float fIndex, xiiUInt8 uiBrightness, float fSaturation /*= 1.0f*/, float fAlpha /*= 1.0f*/)
{
  xiiUInt32 uiIndexA, uiIndexB;
  float     fFrac;
  GetInterpolation(fIndex, uiIndexA, uiIndexB, fFrac);

  const xiiColor a = s_Colors[uiIndexA][uiBrightness];
  const xiiColor b = s_Colors[uiIndexB][uiBrightness];
  const xiiColor c = xiiMath::Lerp(a, b, fFrac);
  const float    l = c.GetLuminance();
  return xiiMath::Lerp(xiiColor(l, l, l), c, fSaturation).WithAlpha(fAlpha);
}

xiiColorScheme::CategoryColorFunc xiiColorScheme::s_CategoryColorFunc = nullptr;

xiiColor xiiColorScheme::GetCategoryColor(xiiStringView sCategory, CategoryColorUsage usage)
{
  if (s_CategoryColorFunc != nullptr)
  {
    return s_CategoryColorFunc(sCategory, usage);
  }

  xiiInt8  iBrightnessOffset = -3;
  xiiUInt8 uiSaturationStep  = 0;

  if (usage == xiiColorScheme::CategoryColorUsage::BorderIconColor)
  {
    // Do not color these icons at all.
    return xiiColor::MakeZero();
  }

  if (usage == xiiColorScheme::CategoryColorUsage::MenuEntryIcon || usage == xiiColorScheme::CategoryColorUsage::AssetMenuIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep  = 0;
  }
  else if (usage == xiiColorScheme::CategoryColorUsage::ViewportIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep  = 2;
  }
  else if (usage == xiiColorScheme::CategoryColorUsage::OverlayIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep  = 0;
  }
  else if (usage == xiiColorScheme::CategoryColorUsage::SceneTreeIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep  = 0;
  }
  else if (usage == xiiColorScheme::CategoryColorUsage::BorderColor)
  {
    iBrightnessOffset = -3;
    uiSaturationStep  = 0;
  }

  const xiiUInt8 uiBrightness = (xiiUInt8)xiiMath::Clamp<xiiInt32>(DarkUIBrightness + iBrightnessOffset, 0, 9);
  const float    fSaturation  = DarkUISaturation - (uiSaturationStep * 0.2f);

  if (const char* szSeparator = sCategory.FindSubString("/"))
  {
    // Chop off everything behind the first separator.
    sCategory = xiiStringView(sCategory.GetStartPointer(), szSeparator);
  }

  if (sCategory.IsEqual_NoCase("AI"))
    return xiiColorScheme::GetColor(xiiColorScheme::Cyan, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Animation"))
    return xiiColorScheme::GetColor(xiiColorScheme::Pink, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Construction"))
    return xiiColorScheme::GetColor(xiiColorScheme::Orange, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Custom") || sCategory.IsEqual_NoCase("Game"))
    return xiiColorScheme::GetColor(xiiColorScheme::Red, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Effects"))
    return xiiColorScheme::GetColor(xiiColorScheme::Grape, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Gameplay"))
    return xiiColorScheme::GetColor(xiiColorScheme::Indigo, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Input"))
    return xiiColorScheme::GetColor(xiiColorScheme::Red, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Lighting"))
    return xiiColorScheme::GetColor(xiiColorScheme::Violet, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Logic"))
    return xiiColorScheme::GetColor(xiiColorScheme::Teal, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Physics"))
    return xiiColorScheme::GetColor(xiiColorScheme::Blue, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Prefabs"))
    return xiiColorScheme::GetColor(xiiColorScheme::Orange, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Rendering"))
    return xiiColorScheme::GetColor(xiiColorScheme::Lime, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Terrain"))
    return xiiColorScheme::GetColor(xiiColorScheme::Lime, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Scripting"))
    return xiiColorScheme::GetColor(xiiColorScheme::Green, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Sound"))
    return xiiColorScheme::GetColor(xiiColorScheme::Blue, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Utilities") || sCategory.IsEqual_NoCase("Editing"))
    return xiiColorScheme::GetColor(xiiColorScheme::Gray, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("XR"))
    return xiiColorScheme::GetColor(xiiColorScheme::Cyan, uiBrightness, fSaturation) * DarkUIFactor;

  xiiLog::Warning("Color for category '{}' is undefined.", sCategory);
  return xiiColor::MakeZero();
}

XII_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_ColorScheme);
