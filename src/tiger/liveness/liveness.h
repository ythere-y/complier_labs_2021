#ifndef TIGER_LIVENESS_LIVENESS_H_
#define TIGER_LIVENESS_LIVENESS_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/x64frame.h"
#include "tiger/liveness/flowgraph.h"
#include "tiger/util/graph.h"
#include <map>
#include <set>

namespace live {
using INode = graph::Node<temp::Temp>;
using INodePtr = graph::Node<temp::Temp> *;
using INodeList = graph::NodeList<temp::Temp>;
using INodeListPtr = graph::NodeList<temp::Temp> *;
using IGraph = graph::Graph<temp::Temp>;
using IGraphPtr = graph::Graph<temp::Temp> *;

#define DEBUG
#ifdef DEBUG
#define LOG(format, args...)                                                   \
  do {                                                                         \
    FILE *debug_log = fopen("register.log", "a+");                             \
    fprintf(debug_log, "%d,%s: ", __LINE__, __func__);                         \
    fprintf(debug_log, format, ##args);                                        \
    fclose(debug_log);                                                         \
  } while (0)
#else
#define LOG(format, args...)                                                   \
  do {                                                                         \
  } while (0)
#endif

class MoveList {
public:
  MoveList() = default;

  [[nodiscard]] const std::list<std::pair<INodePtr, INodePtr>> &
  GetList() const {
    return move_list_;
  }
  void Append(INodePtr src, INodePtr dst) { move_list_.emplace_back(src, dst); }
  bool Contain(INodePtr src, INodePtr dst);
  void Delete(INodePtr src, INodePtr dst);
  void Prepend(INodePtr src, INodePtr dst) {
    move_list_.emplace_front(src, dst);
  }
  MoveList *Union(MoveList *list);
  MoveList *Intersect(MoveList *list);

private:
  std::list<std::pair<INodePtr, INodePtr>> move_list_;
};

struct LiveGraph {
  IGraphPtr interf_graph;
  MoveList *moves;

  LiveGraph(IGraphPtr interf_graph, MoveList *moves)
      : interf_graph(interf_graph), moves(moves) {}
};

static void display(LiveGraph live_graph, INodeListPtr list);
static void display(LiveGraph live_graph);

class LiveGraphFactory {
public:
  explicit LiveGraphFactory(fg::FGraphPtr flowgraph)
      : flowgraph_(flowgraph), live_graph_(new IGraph(), new MoveList()),
        // in_(std::make_unique<graph::Table<assem::Instr,
        // temp::TempList>>()),
        // out_(std::make_unique<graph::Table<assem::Instr,
        // temp::TempList>>()),
        in_(std::make_unique<std::map<fg::FNodePtr, temp::TempList *>>()),
        out_(std::make_unique<std::map<fg::FNodePtr, temp::TempList *>>()),
        temp_node_map_(new tab::Table<temp::Temp, INode>()) {}
  void Liveness();
  LiveGraph GetLiveGraph() { return live_graph_; }
  tab::Table<temp::Temp, INode> *GetTempNodeMap() { return temp_node_map_; }

private:
  fg::FGraphPtr flowgraph_;
  LiveGraph live_graph_;

  // change the table type to std::map because the former is difficult to
  // implement the equal operation std::unique_ptr<graph::Table<assem::Instr,
  // temp::TempList>> in_; std::unique_ptr<graph::Table<assem::Instr,
  // temp::TempList>> out_;

  std::unique_ptr<std::map<fg::FNodePtr, temp::TempList *>> in_;
  std::unique_ptr<std::map<fg::FNodePtr, temp::TempList *>> out_;

  tab::Table<temp::Temp, INode> *temp_node_map_;

  void LiveMap();
  void InterfGraph();

  INodePtr GetNode(temp::Temp *temp);
};

} // namespace live

#endif