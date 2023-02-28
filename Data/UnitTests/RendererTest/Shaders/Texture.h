#include "CommonConstants.h"

struct VS_IN
{
  float3 pos : ATTRIB0;
  float2 texcoord0 : ATTRIB11;
};

struct VS_OUT
{
  float4 pos : SV_Position;
  float2 texcoord0 : TEXCOORD0;
};

typedef VS_OUT PS_IN;
