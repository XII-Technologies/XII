/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <string>

#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>


#include <Foundation/Basics.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/List.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/String.h>

#include <Core/Application/Application.h>
#include <Foundation/Configuration/Startup.h>


class Application : public xiiApplication
{
  void AfterEngineInit() XII_OVERRIDE
  {
    // String and String Builder
    {
      xiiString        bla_xii("bla");
      xiiStringBuilder bla_xii_builder("bla");
      std::string      bla_std = "bla";
    }

    // Dynamic Array
    {
      xiiDynamicArray<xiiString> dynarray_xii;
      dynarray_xii.PushBack("asdf");
      dynarray_xii.PushBack("fads");

      std::vector<std::string> dynarray_std;
      dynarray_std.push_back("asdf");
      dynarray_std.push_back("fads");
    }

    // Linked List
    {
      xiiList<xiiString> linkedlist_xii;
      linkedlist_xii.PushBack("asdf");
      linkedlist_xii.PushBack("fads");

      std::vector<std::string> linkedlist_std;
      linkedlist_std.push_back("asdf");
      linkedlist_std.push_back("fads");
    }

    // Map
    {
      xiiMap<xiiString, xiiString> map_xii;
      map_xii.Insert("asdf", "value");
      map_xii.Insert("fads", "value");

      std::map<std::string, std::string> map_std;
      map_std.insert(std::pair<std::string, std::string>("asdf", "value"));
      map_std.insert(std::pair<std::string, std::string>("fads", "value"));
    }

    // Set
    {
      xiiSet<xiiString> set_xii;
      set_xii.Insert("asdf");
      set_xii.Insert("fads");

      std::set<std::string> set_std;
      set_std.insert("asdf");
      set_std.insert("fads");
    }

    // Hashtable
    {
      xiiHashTable<xiiString, xiiString> hashmap_xii;
      hashmap_xii.Insert("asdf", "value");
      hashmap_xii.Insert("fads", "value"); // currently some troubles here with reading the strings - even in raw view; usage works fine

      std::unordered_map<std::string, xiiString> hashmap_std;
      hashmap_std.insert(std::pair<std::string, xiiString>("asdf", "value"));
      hashmap_std.insert(std::pair<std::string, xiiString>("fads", "value"));
    }

    // Deque - doesn't work yet (chunked based design of xiiDeque makes it difficult)
    /*{
      xiiDeque<xiiString> deque_xii;
      deque_xii.PushBack("asdf");
      deque_xii.PushBack("fads");

      std::deque<std::string> deque_std;
      deque_std.push_back("asdf");
      deque_std.push_back("fads");
    }*/

    // Array
    {
      xiiArrayPtr<int> pArray = XII_DEFAULT_NEW_ARRAY(int, 100);
      XII_DEFAULT_DELETE_ARRAY(pArray);
    }

    // Static Array
    {
      xiiStaticArray<std::string, 2> array_xii;
      array_xii.PushBack("asdf");
      array_xii.PushBack("fads");

      std::array<std::string, 2> array_std;
      array_std[0] = "asdf";
      array_std[1] = "fads";
    }
  }

  virtual ApplicationExecution Run()
  {
    return xiiApplication::Quit;
  }
};

XII_CONSOLEAPP_ENTRY_POINT(Application)
