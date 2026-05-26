#pragma once

#include <DearImguiPlugin/DearImguiPluginDLL.h>

#include <Foundation/Communication/Message.h>

class xiiView;

/// \brief Sent to components or component managers to request them to update their ImGui debug UI for the given view.
///
/// This is sent after render data extraction, so that components can use the extracted data to populate their debug UI if needed.
struct XII_DEARIMGUIPLUGIN_DLL xiiMsgExtractImguiUpdate : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExtractImguiUpdate, xiiMessage);

public:
  const xiiView* m_pView = nullptr; ///< The view for which the render data should be extracted. Components can use this to decide what data to extract based on view properties (e.g. camera usage hint).
};
