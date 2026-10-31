Node %UVTilingOffset
{
  string %Category { "Texturing/Coordinates" }
  string %Color { "Teal" }
  InputPin %UV
  {
    string %Type { "float2" }
    string %DefaultValue { "MaterialInput.TexCoord" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
  }
  InputPin %Tiling
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "1, 1" }
  }
  InputPin %Offset
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "0, 0" }
  }
  OutputPin %UV
  {
    string %Type { "float2" }
    string %Inline { "($in0 * $in1 + $in2)" }
  }
}

Node %RotateAboutPivot
{
  string %Category { "Texturing/Coordinates" }
  string %Color { "Teal" }
  string %CodePixelIncludes { "#include <Shaders/Common/MaterialGraphUtilities.h>" }
  InputPin %UV
  {
    string %Type { "float2" }
    string %DefaultValue { "MaterialInput.TexCoord" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
  }
  InputPin %Pivot
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "0.5, 0.5" }
  }
  InputPin %Angle
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }
  OutputPin %UV
  {
    string %Type { "float2" }
    string %Inline { "(xiiMaterialRotateUV($in0 - $in1, $in2) + $in1)" }
    string %Tooltip { "Angle is in radians." }
  }
}

Node %PolarCoordinates
{
  string %Category { "Texturing/Coordinates" }
  string %Color { "Teal" }
  InputPin %UV
  {
    string %Type { "float2" }
    string %DefaultValue { "MaterialInput.TexCoord" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
  }
  InputPin %Center
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "0.5, 0.5" }
  }
  OutputPin %UV
  {
    string %Type { "float2" }
    string %Inline { "float2(length($in0 - $in1), atan2(($in0 - $in1).y, ($in0 - $in1).x) / 6.2831853f)" }
  }
}

Node %PolarToCartesian
{
  string %Category { "Texturing/Coordinates" }
  string %Color { "Teal" }
  InputPin %Polar
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "0, 0" }
  }
  InputPin %Center
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "0.5, 0.5" }
  }
  OutputPin %UV
  {
    string %Type { "float2" }
    string %Inline { "($in1 + ($in0).x * float2(cos(($in0).y * 6.2831853f), sin(($in0).y * 6.2831853f)))" }
  }
}

Node %MirrorUV
{
  string %Category { "Texturing/Coordinates" }
  string %Color { "Teal" }
  InputPin %UV
  {
    string %Type { "float2" }
    string %DefaultValue { "MaterialInput.TexCoord" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
  }
  OutputPin %UV
  {
    string %Type { "float2" }
    string %Inline { "(1 - abs(frac($in0 * 0.5f) * 2 - 1))" }
  }
}

Node %ParallaxOffset
{
  string %Category { "Texturing/Coordinates" }
  string %Color { "Teal" }
  InputPin %UV
  {
    string %Type { "float2" }
    string %DefaultValue { "MaterialInput.TexCoord" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
  }
  InputPin %Height
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }
  InputPin %TangentView
  {
    string %Type { "float3" }
    bool %Expose { true }
    string %DefaultValue { "0.1, 0.1, 1" }
  }
  InputPin %Scale
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.05" }
  }
  OutputPin %UV
  {
    string %Type { "float2" }
    string %Inline { "($in0 + ($in2).xy / max(abs(($in2).z), 0.1f) * ($in1 - 0.5f) * $in3)" }
  }
}

Node %NormalUnpack
{
  string %Category { "Texturing/Normals" }
  string %Color { "Teal" }
  string %CodePixelIncludes { "#include <Shaders/Common/MaterialGraphUtilities.h>" }
  InputPin %Encoded
  {
    string %Type { "float3" }
    bool %Expose { true }
    string %DefaultValue { "0.5, 0.5, 1" }
  }
  OutputPin %Normal
  {
    string %Type { "float3" }
    string %Inline { "xiiMaterialSafeNormal($in0 * 2 - 1)" }
  }
}

Node %NormalStrength
{
  string %Category { "Texturing/Normals" }
  string %Color { "Teal" }
  string %CodePixelIncludes { "#include <Shaders/Common/MaterialGraphUtilities.h>" }
  InputPin %Normal
  {
    string %Type { "float3" }
    bool %Expose { true }
    string %DefaultValue { "0, 0, 1" }
  }
  InputPin %Strength
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  OutputPin %Normal
  {
    string %Type { "float3" }
    string %Inline { "xiiMaterialSafeNormal(float3(($in0).xy * $in1, ($in0).z))" }
  }
}
