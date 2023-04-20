Node %Parameter1f
{
  string %Category { "Parameters" }
  string %Color { "Red" }

  string %CodeMaterialParams { "float $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "FLOAT1($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "Parameter" }
  }

  Property %Default
  {
    string %Type { "float" }
    string %DefaultValue { "0" }
  }

  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "$prop0" }
  }
}

Node %Parameter2f
{
  string %Category { "Parameters" }
  string %Color { "Red" }

  string %CodeMaterialParams { "float2 $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "FLOAT2($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "Parameter" }
  }

  Property %Default
  {
    string %Type { "float2" }
    string %DefaultValue { "0, 0" }
  }

  OutputPin %Value
  {
    string %Type { "float2" }
    string %Inline { "$prop0" }
  }
}

Node %Parameter3f
{
  string %Category { "Parameters" }
  string %Color { "Red" }

  string %CodeMaterialParams { "float3 $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "FLOAT3($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "Parameter" }
  }

  Property %Default
  {
    string %Type { "float3" }
    string %DefaultValue { "0, 0, 0" }
  }

  OutputPin %Value
  {
    string %Type { "float3" }
    string %Inline { "$prop0" }
  }
}

Node %Parameter4f
{
  string %Category { "Parameters" }
  string %Color { "Red" }

  string %CodeMaterialParams { "float4 $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "FLOAT4($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "Parameter" }
  }

  Property %Default
  {
    string %Type { "float4" }
    string %DefaultValue { "0, 0, 0, 0" }
  }

  OutputPin %Value
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "$prop0" }
  }
}

Node %ParameterColor
{
  string %Category { "Parameters" }
  string %Color { "Red" }

  string %CodeMaterialParams { "Color $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "COLOR4F($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "Parameter" }
  }

  Property %Default
  {
    string %Type { "color" }
    string %DefaultValue { "255, 255, 255, 255" }
  }

  OutputPin %Value
  {
    string %Type { "color" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "$prop0" }
  }
}

Node %ParameterTexture
{
  string %Category { "Parameters" }
  string %NodeType { "Texture" }
  string %Color { "Blue" }

  string %CodeMaterialParams { "Texture2D $prop0 @Default(\"$prop0\");" }

  string %CodePixelSamplers { "
Texture2D $prop0;
SamplerState $prop0_AutoSampler;
" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "Parameter" }
  }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %DefaultValue { "RebeccaPurple.color" }
  }

  InputPin %UV
  {
    string %Color { "Teal" }
    string %Type { "float2" }
    string %DefaultValue { "G.Input.TexCoord0" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  OutputPin %RGBA
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "$prop0.Sample($prop0_AutoSampler, ToFloat2($in0))" }
  }

  OutputPin %Red
  {
    string %Type { "float" }
    string %Color { "Red" }
    string %Inline { "$prop0.Sample($prop0_AutoSampler, ToFloat2($in0)).x" }
  }

  OutputPin %Green
  {
    string %Type { "float" }
    string %Color { "Green" }
    string %Inline { "$prop0.Sample($prop0_AutoSampler, ToFloat2($in0)).y" }
  }

  OutputPin %Blue
  {
    string %Type { "float" }
    string %Color { "Blue" }
    string %Inline { "$prop0.Sample($prop0_AutoSampler, ToFloat2($in0)).z" }
  }

  OutputPin %Alpha
  {
    string %Type { "float" }
    string %Inline { "$prop0.Sample($prop0_AutoSampler, ToFloat2($in0)).w" }
  }
}
