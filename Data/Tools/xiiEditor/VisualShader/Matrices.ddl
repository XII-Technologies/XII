Node %FromCameraSpace
{
  string %Category { "Transformations" }
  string %Color { "Indigo" }

  InputPin %CameraSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Tooltip { "Position in camera-space." }
  }

  OutputPin %ScreenSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "xiiMaterialProject(MaterialViewToClip, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in screen-space." }
  }

  OutputPin %WorldSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "mul(MaterialViewToWorld, ToFloat4Position($in0)).xyz" }
    string %Tooltip { "Transformed position in world-space." }
  }
}

Node %FromScreenSpace
{
  string %Category { "Transformations" }
  string %Color { "Indigo" }

  InputPin %ScreenSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Tooltip { "Position in screen-space." }
  }

  OutputPin %CameraSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "xiiMaterialProject(MaterialClipToView, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in camera-space." }
  }

  OutputPin %WorldSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "xiiMaterialProject(mul(MaterialViewToWorld, MaterialClipToView), ToFloat3($in0))" }
    string %Tooltip { "Transformed position in world-space." }
  }
}

Node %FromWorldSpace
{
  string %Category { "Transformations" }
  string %Color { "Indigo" }

  InputPin %WorldSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Tooltip { "Position in world-space." }
  }

  OutputPin %ObjectSpacePos
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "xiiMaterialWorldToObject(g_SceneInstances[MaterialInput.InstanceIndex], ToFloat3($in0), true)" }
    string %Tooltip { "Transformed position in object-space." }
  }  

  OutputPin %ObjectSpaceDir
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %Inline { "xiiMaterialWorldToObject(g_SceneInstances[MaterialInput.InstanceIndex], ToFloat3($in0), false)" }
    string %Tooltip { "Transformed direction vector in object-space." }
  }
  OutputPin %CameraSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "mul(MaterialWorldToView, ToFloat4Position($in0)).xyz" }
    string %Tooltip { "Transformed position in camera-space." }
  }

  OutputPin %ScreenSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "xiiMaterialProject(ViewProjectionMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in screen-space." }
  }
}

Node %FromObjectSpace
{
  string %Category { "Transformations" }
  string %Color { "Indigo" }

  InputPin %ObjectSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Tooltip { "Position or direction vector in object-space." }
  }

  OutputPin %WorldPosition
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "mul(g_SceneInstances[MaterialInput.InstanceIndex].GlobalTransform, ToFloat4Position($in0)).xyz" }
    string %Tooltip { "Transformed position in world-space." }
  }

  OutputPin %WorldDirection
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %Inline { "mul((float3x3)g_SceneInstances[MaterialInput.InstanceIndex].GlobalTransform, ToFloat3($in0))" }
    string %Tooltip { "Transformed direction vector in world-space." }
  }
}

Node %TangentToWorldSpace
{
  string %Category { "Transformations" }
  string %Color { "Indigo" }

  InputPin %TangentSpace
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %Tooltip { "Normal in tangent space." }
  }

  OutputPin %WorldSpace
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %Inline { "xiiMaterialTangentToWorld(MaterialInput, ToFloat3($in0))" }
    string %Tooltip { "Transformed normal in world-space." }
  }
}


