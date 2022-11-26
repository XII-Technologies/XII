#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/DGMLWriter.h>

const char* xiiWorkerThreadType::GetThreadTypeName(xiiWorkerThreadType::Enum ThreadType)
{
  switch (ThreadType)
  {
    case xiiWorkerThreadType::ShortTasks:
      return "Short Task";

    case xiiWorkerThreadType::LongTasks:
      return "Long Task";

    case xiiWorkerThreadType::FileAccess:
      return "File Access";

    default:
      XII_REPORT_FAILURE("Invalid Thread Type");
      return "unknown";
  }
}

void xiiTaskSystem::WriteStateSnapshotToDGML(xiiDGMLGraph& graph)
{
  XII_LOCK(s_TaskSystemMutex);

  xiiHashTable<const xiiTaskGroup*, xiiDGMLGraph::NodeId> groupNodeIds;

  xiiStringBuilder title, tmp;

  xiiDGMLGraph::NodeDesc taskGroupND;
  taskGroupND.m_Color = xiiColor::CornflowerBlue;
  taskGroupND.m_Shape = xiiDGMLGraph::NodeShape::Rectangle;

  xiiDGMLGraph::NodeDesc taskNodeND;
  taskNodeND.m_Color = xiiColor::OrangeRed;
  taskNodeND.m_Shape = xiiDGMLGraph::NodeShape::RoundedRectangle;

  const xiiDGMLGraph::PropertyId startedByUserId = graph.AddPropertyType("StartByUser");
  const xiiDGMLGraph::PropertyId activeDepsId    = graph.AddPropertyType("ActiveDependencies");
  const xiiDGMLGraph::PropertyId scheduledId     = graph.AddPropertyType("Scheduled");
  const xiiDGMLGraph::PropertyId finishedId      = graph.AddPropertyType("Finished");
  const xiiDGMLGraph::PropertyId multiplicityId  = graph.AddPropertyType("Multiplicity");
  const xiiDGMLGraph::PropertyId remainingRunsId = graph.AddPropertyType("RemainingRuns");
  const xiiDGMLGraph::PropertyId priorityId      = graph.AddPropertyType("GroupPriority");

  const char* szTaskPriorityNames[xiiTaskPriority::ENUM_COUNT]  = {};
  szTaskPriorityNames[xiiTaskPriority::EarlyThisFrame]          = "EarlyThisFrame";
  szTaskPriorityNames[xiiTaskPriority::ThisFrame]               = "ThisFrame";
  szTaskPriorityNames[xiiTaskPriority::LateThisFrame]           = "LateThisFrame";
  szTaskPriorityNames[xiiTaskPriority::EarlyNextFrame]          = "EarlyNextFrame";
  szTaskPriorityNames[xiiTaskPriority::NextFrame]               = "NextFrame";
  szTaskPriorityNames[xiiTaskPriority::LateNextFrame]           = "LateNextFrame";
  szTaskPriorityNames[xiiTaskPriority::In2Frames]               = "In 2 Frames";
  szTaskPriorityNames[xiiTaskPriority::In2Frames]               = "In 3 Frames";
  szTaskPriorityNames[xiiTaskPriority::In4Frames]               = "In 4 Frames";
  szTaskPriorityNames[xiiTaskPriority::In5Frames]               = "In 5 Frames";
  szTaskPriorityNames[xiiTaskPriority::In6Frames]               = "In 6 Frames";
  szTaskPriorityNames[xiiTaskPriority::In7Frames]               = "In 7 Frames";
  szTaskPriorityNames[xiiTaskPriority::In8Frames]               = "In 8 Frames";
  szTaskPriorityNames[xiiTaskPriority::In9Frames]               = "In 9 Frames";
  szTaskPriorityNames[xiiTaskPriority::LongRunningHighPriority] = "LongRunningHighPriority";
  szTaskPriorityNames[xiiTaskPriority::LongRunning]             = "LongRunning";
  szTaskPriorityNames[xiiTaskPriority::FileAccessHighPriority]  = "FileAccessHighPriority";
  szTaskPriorityNames[xiiTaskPriority::FileAccess]              = "FileAccess";
  szTaskPriorityNames[xiiTaskPriority::ThisFrameMainThread]     = "ThisFrameMainThread";
  szTaskPriorityNames[xiiTaskPriority::SomeFrameMainThread]     = "SomeFrameMainThread";

  for (xiiUInt32 g = 0; g < s_pState->m_TaskGroups.GetCount(); ++g)
  {
    const xiiTaskGroup& tg = s_pState->m_TaskGroups[g];

    if (!tg.m_bInUse)
      continue;

    title.Format("Group {}", g);

    const xiiDGMLGraph::NodeId taskGroupId = graph.AddGroup(title, xiiDGMLGraph::GroupType::Expanded, &taskGroupND);
    groupNodeIds[&tg]                      = taskGroupId;

    graph.AddNodeProperty(taskGroupId, startedByUserId, tg.m_bStartedByUser ? "true" : "false");
    graph.AddNodeProperty(taskGroupId, priorityId, szTaskPriorityNames[tg.m_Priority]);
    graph.AddNodeProperty(taskGroupId, activeDepsId, xiiFmt("{}", tg.m_iNumActiveDependencies));

    for (xiiUInt32 t = 0; t < tg.m_Tasks.GetCount(); ++t)
    {
      const xiiTask&             task       = *tg.m_Tasks[t];
      const xiiDGMLGraph::NodeId taskNodeId = graph.AddNode(task.m_sTaskName, &taskNodeND);

      graph.AddNodeToGroup(taskNodeId, taskGroupId);

      graph.AddNodeProperty(taskNodeId, scheduledId, task.m_bTaskIsScheduled ? "true" : "false");
      graph.AddNodeProperty(taskNodeId, finishedId, task.IsTaskFinished() ? "true" : "false");

      tmp.Format("{}", task.GetMultiplicity());
      graph.AddNodeProperty(taskNodeId, multiplicityId, tmp);

      tmp.Format("{}", task.m_iRemainingRuns);
      graph.AddNodeProperty(taskNodeId, remainingRunsId, tmp);
    }
  }

  for (xiiUInt32 g = 0; g < s_pState->m_TaskGroups.GetCount(); ++g)
  {
    const xiiTaskGroup& tg = s_pState->m_TaskGroups[g];

    if (!tg.m_bInUse)
      continue;

    const xiiDGMLGraph::NodeId ownNodeId = groupNodeIds[&tg];

    for (const xiiTaskGroupID& dependsOn : tg.m_DependsOnGroups)
    {
      xiiDGMLGraph::NodeId otherNodeId;

      // filter out already fulfilled dependencies
      if (dependsOn.m_pTaskGroup->m_uiGroupCounter != dependsOn.m_uiGroupCounter)
        continue;

      // filter out already fulfilled dependencies
      if (!groupNodeIds.TryGetValue(dependsOn.m_pTaskGroup, otherNodeId))
        continue;

      XII_ASSERT_DEBUG(otherNodeId != ownNodeId, "");

      graph.AddConnection(otherNodeId, ownNodeId);
    }
  }
}

void xiiTaskSystem::WriteStateSnapshotToFile(const char* szPath /*= nullptr*/)
{
  xiiStringBuilder sPath = szPath;

  if (sPath.IsEmpty())
  {
    sPath = ":appdata/TaskGraphs/";

    const xiiDateTime dt = xiiTimestamp::CurrentTimestamp();

    sPath.AppendFormat("{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), xiiArgU(dt.GetMonth(), 2, true), xiiArgU(dt.GetDay(), 2, true), xiiArgU(dt.GetHour(), 2, true), xiiArgU(dt.GetMinute(), 2, true), xiiArgU(dt.GetSecond(), 2, true), xiiArgU(dt.GetMicroseconds() / 1000, 3, true));

    sPath.ChangeFileExtension("dgml");
  }

  xiiDGMLGraph graph;
  xiiTaskSystem::WriteStateSnapshotToDGML(graph);

  xiiDGMLGraphWriter::WriteGraphToFile(sPath, graph).IgnoreResult();

  xiiStringBuilder absPath;
  xiiFileSystem::ResolvePath(sPath, &absPath, nullptr).IgnoreResult();
  xiiLog::Info("Task graph snapshot saved to '{}'", absPath);
}


XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystemUtils);
