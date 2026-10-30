Node %OneMinus
{
  string %Category { "Math/Basic" }
  string %Color { "Teal" }
  InputPin %Value
  {
    string %Type { "numeric" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }
  OutputPin %Value
  {
    string %Type { "numeric" }
    string %Inline { "(1 - $in0)" }
  }
}

Node %SafeReciprocal
{
  string %Category { "Math/Basic" }
  string %Color { "Teal" }
  InputPin %Value
  {
    string %Type { "numeric" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  OutputPin %Value
  {
    string %Type { "numeric" }
    string %Inline { "(sign(($in0) + 0.0f) / max(abs($in0), 0.00001f))" }
    string %Tooltip { "Zero maps to zero. Denominators are bounded away from zero." }
  }
}

Node %SafeDivide
{
  string %Category { "Math/Basic" }
  string %Color { "Teal" }
  InputPin %Numerator
  {
    string %Type { "numeric" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  InputPin %Denominator
  {
    string %Type { "numeric" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  OutputPin %Value
  {
    string %Type { "numeric" }
    string %Inline { "($in0 * sign(($in1) + 0.0f) / max(abs($in1), 0.00001f))" }
  }
}

Node %Smoothstep
{
  string %Category { "Math/Interpolation" }
  string %Color { "Teal" }
  InputPin %Value
  {
    string %Type { "numeric" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }
  InputPin %Low
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }
  InputPin %High
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  OutputPin %Value
  {
    string %Type { "numeric" }
    string %Inline { "smoothstep($in1, max($in2, $in1 + 0.00001f), $in0)" }
  }
}

Node %Step
{
  string %Category { "Math/Comparisons" }
  string %Color { "Teal" }
  InputPin %Value
  {
    string %Type { "numeric" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }
  InputPin %Edge
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }
  OutputPin %Value
  {
    string %Type { "numeric" }
    string %Inline { "step($in1, $in0)" }
  }
}

Node %InverseLerp
{
  string %Category { "Math/Interpolation" }
  string %Color { "Teal" }
  InputPin %Value
  {
    string %Type { "numeric" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }
  InputPin %Low
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }
  InputPin %High
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  OutputPin %Value
  {
    string %Type { "numeric" }
    string %Inline { "(($in0 - $in1) * sign(($in2 - $in1) + 0.0f) / max(abs($in2 - $in1), 0.00001f))" }
  }
}

Node %Remap
{
  string %Category { "Math/Interpolation" }
  string %Color { "Teal" }
  InputPin %Value
  {
    string %Type { "numeric" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }
  InputPin %InputLow
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }
  InputPin %InputHigh
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  InputPin %OutputLow
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }
  InputPin %OutputHigh
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  OutputPin %Value
  {
    string %Type { "numeric" }
    string %Inline { "($in3 + ($in4 - $in3) * ($in0 - $in1) * sign(($in2 - $in1) + 0.0f) / max(abs($in2 - $in1), 0.00001f))" }
  }
}

Node %PingPong
{
  string %Category { "Math/Repeating" }
  string %Color { "Teal" }
  InputPin %Value
  {
    string %Type { "numeric" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }
  OutputPin %Value
  {
    string %Type { "numeric" }
    string %Inline { "(1 - abs(frac($in0 * 0.5f) * 2 - 1))" }
  }
}

Node %Posterize
{
  string %Category { "Math/Repeating" }
  string %Color { "Teal" }
  InputPin %Value
  {
    string %Type { "numeric" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }
  InputPin %Levels
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "8" }
  }
  OutputPin %Value
  {
    string %Type { "numeric" }
    string %Inline { "(floor(saturate($in0) * (max(floor($in1), 2) - 1) + 0.5f) / (max(floor($in1), 2) - 1))" }
  }
}

Node %ModuloPositive
{
  string %Category { "Math/Repeating" }
  string %Color { "Teal" }
  InputPin %Value
  {
    string %Type { "numeric" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }
  InputPin %Period
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  OutputPin %Value
  {
    string %Type { "numeric" }
    string %Inline { "(frac($in0 / max(abs($in1), 0.00001f)) * max(abs($in1), 0.00001f))" }
  }
}
