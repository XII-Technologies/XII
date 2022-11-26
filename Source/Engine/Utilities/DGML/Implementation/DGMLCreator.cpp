#include <Utilities/UtilitiesPCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <Utilities/DGML/DGMLCreator.h>

void xiiDGMLGraphCreator::FillGraphFromWorld(xiiWorld* pWorld, xiiDGMLGraph& Graph)
{
  if (!pWorld)
  {
    xiiLog::Warning("xiiDGMLGraphCreator::FillGraphFromWorld() called with null world!");
    return;
  }


  struct GraphVisitor
  {
    GraphVisitor(xiiDGMLGraph& Graph) :
      m_Graph(Graph)
    {
      xiiDGMLGraph::NodeDesc nd;
      nd.m_Color    = xiiColor::DarkRed;
      nd.m_Shape    = xiiDGMLGraph::NodeShape::Button;
      m_WorldNodeId = Graph.AddNode("World", &nd);
    }

    xiiVisitorExecution::Enum Visit(xiiGameObject* pObject)
    {
      xiiStringBuilder name;
      name.Format("GameObject: \"{0}\"", xiiStringUtils::IsNullOrEmpty(pObject->GetName()) ? "<Unnamed>" : pObject->GetName());

      // Create node for game object
      xiiDGMLGraph::NodeDesc gameobjectND;
      gameobjectND.m_Color  = xiiColor::CornflowerBlue;
      gameobjectND.m_Shape  = xiiDGMLGraph::NodeShape::Rectangle;
      auto gameObjectNodeId = m_Graph.AddNode(name.GetData(), &gameobjectND);

      m_VisitedObjects.Insert(pObject, gameObjectNodeId);

      // Add connection to parent if existent
      if (const xiiGameObject* parent = pObject->GetParent())
      {
        auto it = m_VisitedObjects.Find(parent);

        if (it.IsValid())
        {
          m_Graph.AddConnection(gameObjectNodeId, it.Value());
        }
      }
      else
      {
        // No parent -> connect to world
        m_Graph.AddConnection(gameObjectNodeId, m_WorldNodeId);
      }

      // Add components
      for (auto component : pObject->GetComponents())
      {
        auto szComponentName = component->GetDynamicRTTI()->GetTypeName();

        xiiDGMLGraph::NodeDesc componentND;
        componentND.m_Color  = xiiColor::LimeGreen;
        componentND.m_Shape  = xiiDGMLGraph::NodeShape::RoundedRectangle;
        auto componentNodeId = m_Graph.AddNode(szComponentName, &componentND);

        // And add the link to the game object

        m_Graph.AddConnection(componentNodeId, gameObjectNodeId);
      }

      return xiiVisitorExecution::Continue;
    }

    xiiDGMLGraph& m_Graph;

    xiiDGMLGraph::NodeId                               m_WorldNodeId;
    xiiMap<const xiiGameObject*, xiiDGMLGraph::NodeId> m_VisitedObjects;
  };

  GraphVisitor visitor(Graph);
  pWorld->Traverse(xiiWorld::VisitorFunc(&GraphVisitor::Visit, &visitor), xiiWorld::BreadthFirst);
}



XII_STATICLINK_FILE(Utilities, Utilities_DGML_Implementation_DGMLCreator);
