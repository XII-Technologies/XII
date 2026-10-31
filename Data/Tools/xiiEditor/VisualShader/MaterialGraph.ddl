Node %Reroute
{
  string %Category { "Organization" }
  string %Color { "Gray" }
  InputPin %Value
  {
    string %Type { "numeric" }
  }
  OutputPin %Value
  {
    string %Type { "numeric" }
    string %Inline { "($in0)" }
  }
}

Node %Comment
{
  string %Category { "Organization" }
  string %Color { "Gray" }
  Property %Comment
  {
    string %Type { "string" }
    string %DefaultValue { "Comment" }
  }
}

Node %UVTransform
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
  InputPin %Scale
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
  InputPin %Rotation
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
    string %Tooltip { "Rotation in radians." }
  }
  OutputPin %UV
  {
    string %Type { "float2" }
    string %Inline { "xiiMaterialRotateUV($in0 * $in1, $in3) + $in2" }
  }
}

Node %ValueNoise2D
{
  string %Category { "Procedural" }
  string %Color { "Green" }
  string %CodePixelIncludes { "#include <Shaders/Common/MaterialGraphUtilities.h>" }
  InputPin %Position
  {
    string %Type { "float2" }
    string %DefaultValue { "MaterialInput.TexCoord" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
  }
  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "xiiMaterialValueNoise2D($in0)" }
  }
}

Node %LinearGradient
{
  string %Category { "Procedural" }
  string %Color { "Green" }
  InputPin %Position
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
  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "saturate(dot($in0, $in1))" }
  }
}
