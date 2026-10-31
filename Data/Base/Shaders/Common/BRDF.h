/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/Common.h>

// TODO:
// check roughness^2 vs. roughness^4
// try different visibility function (SmithGGXCorrelated)

float RoughnessFromMipLevel(uint mipLevel, uint mipCount)
{
  return pow(mipLevel / (float)(mipCount - 1), 2.0f);
}

float MipLevelFromRoughness(float roughness, uint mipCount)
{
  return (mipCount - 1) * sqrt(roughness);
}

float RoughnessFromPerceptualRoughness(float perceptualRoughness)
{
  return perceptualRoughness * perceptualRoughness;
}

float PerceptualRoughnessFromRoughness(float roughness)
{
  return sqrt(roughness);
}

float3 DiffuseLambert(float3 diffuseColor)
{
  return diffuseColor;
}

// divide by PI postponed
float SpecularGGX(float roughness, float NdotH)
{
  // mad friendly reformulation of:
  //
  //              a^2
  // --------------------------------
  // PI * ((N.H)^2 * (a^2 - 1) + 1)^2

  float a2 = roughness * roughness;
  float f  = (NdotH * a2 - NdotH) * NdotH + 1.0f;
  return a2 / (f * f);
}

float VisibilitySmithCorrelated(float roughness, float NdotV, float NdotL)
{
  float a2      = roughness * roughness;
  float lambdaV = NdotL * sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
  float lambdaL = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);
  return 0.5f / (lambdaV + lambdaL);
}

float VisibilitySmithJointApprox(float roughness, float NdotV, float NdotL)
{
  float a       = roughness;
  float b       = 1.0f - a;
  float lambdaV = NdotL * (NdotV * b + a);
  float lambdaL = NdotV * (NdotL * b + a);
  return 0.5f / (lambdaV + lambdaL);
}

float3 FresnelSchlick(float3 specularColor, float u)
{
  float f  = 1.0f - u;
  float ff = f * f;
  float f5 = ff * ff * f;

  // specularColor below 2% is considered to be shadowing
  return saturate(50.0 * GetLuminance(specularColor)) * f5 + (1 - f5) * specularColor;
}

