#pragma once

#include "../Common/Platforms.h"

#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(xiiTonemapConstants, 3)
{
  FLOAT4(AutoExposureParams);

  COLOR4F(MoodColor);
  FLOAT1(MoodStrength);
  FLOAT1(Saturation);
  FLOAT1(Lut1Strength);
  FLOAT1(Lut2Strength);
  FLOAT4(ContrastParams);
};

CONSTANT_BUFFER(xiiAdvancedTonemapConstants, 3)
{
  // Tone mapping mode.
  INT1(ToneMappingMode);
  // Automatically compute the exposure to use in tone mapping.
  BOOL1(AutoExposure);
  // Middle gray value used by tone mapping operators.
  FLOAT1(MiddleGray);
  // Simulate eye adaptation to light changes.
  BOOL1(LightAdaptation);

  // White point to use in tone mapping.
  FLOAT1(WhitePoint);
  // Luminance point to use in tone mapping.
  FLOAT1(LuminanceSaturation);

  FLOAT1(AverageLogLum);

  UINT1(Padding0);
};

#define TONE_MAPPING_MODE_EXP          0
#define TONE_MAPPING_MODE_REINHARD     1
#define TONE_MAPPING_MODE_REINHARD_MOD 2
#define TONE_MAPPING_MODE_UNCHARTED2   3
#define TONE_MAPPING_FILMIC_ALU        4
#define TONE_MAPPING_LOGARITHMIC       5
#define TONE_MAPPING_ADAPTIVE_LOG      6
#define TONE_MAPPING_LOTTES            7
#define TONE_MAPPING_UCHIMURA          8
#define TONE_MAPPING_UNREAL            9
#define TONE_MAPPING_ACES              10
