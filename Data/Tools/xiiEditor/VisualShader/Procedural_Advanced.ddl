Node %Checkerboard
{
  string %Category { "Procedural/Patterns" }
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
    string %DefaultValue { "8, 8" }
  }
  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "fmod(abs(floor(($in0 * $in1).x) + floor(($in0 * $in1).y)), 2)" }
  }
}

Node %RadialGradient
{
  string %Category { "Procedural/Gradients" }
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
  InputPin %Radius
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }
  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "saturate(1 - length($in0 - $in1) / max(abs($in2), 0.00001f))" }
  }
}

Node %RingGradient
{
  string %Category { "Procedural/Gradients" }
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
  InputPin %Radius
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.3" }
  }
  InputPin %Width
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.05" }
  }
  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "saturate(1 - abs(length($in0 - $in1) - $in2) / max(abs($in3), 0.00001f))" }
  }
}

Node %StripePattern
{
  string %Category { "Procedural/Patterns" }
  string %Color { "Teal" }
  InputPin %UV
  {
    string %Type { "float2" }
    string %DefaultValue { "MaterialInput.TexCoord" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
  }
  InputPin %Direction
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "1, 0" }
  }
  InputPin %Frequency
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "8" }
  }
  InputPin %DutyCycle
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }
  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "step(frac(dot($in0, $in1) * $in2), saturate($in3))" }
  }
}

Node %Voronoi2D
{
  string %Category { "Procedural/Noise" }
  string %Color { "Teal" }
  string %CodePixelIncludes { "#include <Shaders/Common/MaterialGraphUtilities.h>" }
  InputPin %UV
  {
    string %Type { "float2" }
    string %DefaultValue { "MaterialInput.TexCoord" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
  }
  InputPin %Scale
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "8" }
  }
  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "xiiMaterialVoronoi2D($in0 * $in1)" }
  }
}

Node %FractalNoise2D
{
  string %Category { "Procedural/Noise" }
  string %Color { "Teal" }
  string %CodePixelIncludes { "#include <Shaders/Common/MaterialGraphUtilities.h>" }
  InputPin %UV
  {
    string %Type { "float2" }
    string %DefaultValue { "MaterialInput.TexCoord" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
  }
  InputPin %Scale
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "8" }
  }
  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "xiiMaterialFractalNoise2D($in0 * $in1)" }
    string %Tooltip { "Four fixed octaves of smooth value noise. No extra permutations." }
  }
}

Node %RidgedNoise2D
{
  string %Category { "Procedural/Noise" }
  string %Color { "Teal" }
  string %CodePixelIncludes { "#include <Shaders/Common/MaterialGraphUtilities.h>" }
  InputPin %UV
  {
    string %Type { "float2" }
    string %DefaultValue { "MaterialInput.TexCoord" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
  }
  InputPin %Scale
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "8" }
  }
  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "(1 - abs(xiiMaterialFractalNoise2D($in0 * $in1) * 2 - 1))" }
  }
}

Node %Turbulence2D
{
  string %Category { "Procedural/Noise" }
  string %Color { "Teal" }
  string %CodePixelIncludes { "#include <Shaders/Common/MaterialGraphUtilities.h>" }
  InputPin %UV
  {
    string %Type { "float2" }
    string %DefaultValue { "MaterialInput.TexCoord" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
  }
  InputPin %Scale
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "8" }
  }
  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "abs(xiiMaterialFractalNoise2D($in0 * $in1) * 2 - 1)" }
  }
}
