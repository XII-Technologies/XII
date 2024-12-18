#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/VirtualThumbStick.h>
#include <Foundation/Math/Rect.h>
#include <GameEngine/Input/InputDebugVis.h>
#include <GraphicsCore/Debug/DebugRenderer.h>

void xiiInputDebugVis::DebugRender(const xiiDebugRendererContext& context, const xiiVec2& vResolution, const xiiVirtualThumbStick& stick)
{
  if (!stick.IsEnabled())
    return;

  const bool bActive = stick.IsActive();

  xiiVec2 vLL, vUR;
  stick.GetInputArea(vLL, vUR);
  const xiiVec2 vAreaSize = vUR - vLL;

  const xiiVec2 vCenter   = stick.GetCurrentCenter();
  const xiiVec2 vTouchPos = stick.GetCurrentTouchPos();
  const float   fRadius   = stick.GetThumbstickRadius();
  const float   fStrength = stick.GetInputStrength();
  const float   fAspect   = stick.GetInputCoordinateAspectRatio();

  xiiRectFloat area;
  area.x      = vLL.x * vResolution.x;
  area.y      = vLL.y * vResolution.y;
  area.width  = vAreaSize.x * vResolution.x;
  area.height = vAreaSize.y * vResolution.y;

  xiiDebugRenderer::Draw2DLineRectangle(context, area, 0.0f, bActive ? xiiColor::Yellow : xiiColor::Grey);

  area.x      = (vCenter.x - fRadius) * vResolution.x;
  area.y      = (vCenter.y * vResolution.y) - (fRadius * vResolution.y * fAspect);
  area.width  = fRadius * 2 * vResolution.x;
  area.height = fRadius * 2 * vResolution.y * fAspect;
  xiiDebugRenderer::Draw2DLineRectangle(context, area, 0.0f, bActive ? xiiColor::GreenYellow : xiiColor::Yellow);

  if (bActive)
  {
    const float size = 0.03f;
    area.x           = (vTouchPos.x - size) * vResolution.x;
    area.y           = (vTouchPos.y * vResolution.y) - (size * vResolution.y * fAspect);
    area.width       = size * 2 * vResolution.x;
    area.height      = size * 2 * vResolution.y * fAspect;
    xiiDebugRenderer::Draw2DRectangle(context, area, 0.0f, xiiColor::OrangeRed);


    xiiVec2I32 pos;
    pos.x = xiiMath::RoundToInt(vCenter.x * vResolution.x);
    pos.y = xiiMath::RoundToInt(vCenter.y * vResolution.y);

    xiiDebugRenderer::Draw2DText(context, xiiFmt("{}", xiiArgF(fStrength, 2)), pos, xiiColor::OrangeRed, 16, xiiDebugTextHAlign::Center, xiiDebugTextVAlign::Center);
  }
}
