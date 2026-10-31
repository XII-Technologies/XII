Node %Luminance
{
  string %Category { "Color/Adjustments" }
  string %Color { "Teal" }
  InputPin %Color
  {
    string %Type { "color" }
    bool %Expose { true }
    string %DefaultValue { "255, 255, 255, 255" }
  }
  OutputPin %Value
  {
    string %Type { "float" }
    string %Inline { "dot(($in0).rgb, float3(0.2126f, 0.7152f, 0.0722f))" }
  }
}

Node %ColorRGB
{
  string %Category { "Components" }
  string %Color { "Teal" }
  InputPin %Color
  {
    string %Type { "color" }
    bool %Expose { true }
    string %DefaultValue { "255, 255, 255, 255" }
  }
  OutputPin %Value
  {
    string %Type { "float3" }
    string %Inline { "($in0).rgb" }
  }
}

Node %Desaturate
{
  string %Category { "Color/Adjustments" }
  string %Color { "Teal" }
  InputPin %Color
  {
    string %Type { "color" }
    bool %Expose { true }
    string %DefaultValue { "255, 255, 255, 255" }
  }
  InputPin %Amount
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  OutputPin %Color
  {
    string %Type { "color" }
    string %Inline { "float4(lerp(($in0).rgb, dot(($in0).rgb, float3(0.2126f, 0.7152f, 0.0722f)).xxx, saturate($in1)), ($in0).a)" }
  }
}

Node %ColorContrast
{
  string %Category { "Color/Adjustments" }
  string %Color { "Teal" }
  InputPin %Color
  {
    string %Type { "color" }
    bool %Expose { true }
    string %DefaultValue { "255, 255, 255, 255" }
  }
  InputPin %Contrast
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  InputPin %Pivot
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.18" }
  }
  OutputPin %Color
  {
    string %Type { "color" }
    string %Inline { "float4(max((($in0).rgb - $in2) * $in1 + $in2, 0), ($in0).a)" }
  }
}

Node %BlendScreen
{
  string %Category { "Color/Blending" }
  string %Color { "Teal" }
  InputPin %Base
  {
    string %Type { "color" }
    bool %Expose { true }
    string %DefaultValue { "255, 255, 255, 255" }
  }
  InputPin %Layer
  {
    string %Type { "color" }
    bool %Expose { true }
    string %DefaultValue { "255, 255, 255, 255" }
  }
  InputPin %Amount
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  OutputPin %Color
  {
    string %Type { "color" }
    string %Inline { "float4(lerp(($in0).rgb, 1 - (1 - ($in0).rgb) * (1 - ($in1).rgb), saturate($in2)), ($in0).a)" }
  }
}

Node %BlendOverlay
{
  string %Category { "Color/Blending" }
  string %Color { "Teal" }
  InputPin %Base
  {
    string %Type { "color" }
    bool %Expose { true }
    string %DefaultValue { "255, 255, 255, 255" }
  }
  InputPin %Layer
  {
    string %Type { "color" }
    bool %Expose { true }
    string %DefaultValue { "255, 255, 255, 255" }
  }
  InputPin %Amount
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  OutputPin %Color
  {
    string %Type { "color" }
    string %Inline { "float4(lerp(($in0).rgb, lerp(2 * ($in0).rgb * ($in1).rgb, 1 - 2 * (1 - ($in0).rgb) * (1 - ($in1).rgb), step(0.5f, ($in0).rgb)), saturate($in2)), ($in0).a)" }
  }
}

Node %BlendSoftLight
{
  string %Category { "Color/Blending" }
  string %Color { "Teal" }
  InputPin %Base
  {
    string %Type { "color" }
    bool %Expose { true }
    string %DefaultValue { "255, 255, 255, 255" }
  }
  InputPin %Layer
  {
    string %Type { "color" }
    bool %Expose { true }
    string %DefaultValue { "255, 255, 255, 255" }
  }
  InputPin %Amount
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  OutputPin %Color
  {
    string %Type { "color" }
    string %Inline { "float4(lerp(($in0).rgb, (1 - 2 * ($in1).rgb) * ($in0).rgb * ($in0).rgb + 2 * ($in1).rgb * ($in0).rgb, saturate($in2)), ($in0).a)" }
  }
}

Node %HeightBlend
{
  string %Category { "Surface/Layering" }
  string %Color { "Teal" }
  InputPin %Base
  {
    string %Type { "color" }
    bool %Expose { true }
    string %DefaultValue { "255, 255, 255, 255" }
  }
  InputPin %Layer
  {
    string %Type { "color" }
    bool %Expose { true }
    string %DefaultValue { "255, 255, 255, 255" }
  }
  InputPin %BaseHeight
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }
  InputPin %LayerHeight
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }
  InputPin %Amount
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }
  InputPin %Transition
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.1" }
  }
  OutputPin %Color
  {
    string %Type { "color" }
    string %Inline { "lerp($in0, $in1, saturate(($in3 - $in2 + $in4 * 2 - 1) / max($in5, 0.00001f) + 0.5f))" }
  }
}
