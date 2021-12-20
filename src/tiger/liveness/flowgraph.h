#ifndef TIGER_LIVENESS_FLOWGRAPH_H_
#define TIGER_LIVENESS_FLOWGRAPH_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/util/graph.h"

// #define FDEBUG
#ifdef FDEBUG
#define FLOG(format, args...)                                                  \
  do {                                                                         \
    FILE *debug_log = fopen("register.log", "a+");                             \
    fprintf(debug_log, "%d,%s: ", __LINE__, __func__);                         \
    fprintf(debug_log, format, ##args);                                        \
    fclose(debug_log);                                                         \
  } while (0)
#else
#define FLOG(format, args...)                                                  \
  do {                                                                         \
  } while (0)
#endif

namespace fg {

using FNode = graph::Node<assem::Instr>;
using FNodePtr = graph::Node<assem::Instr> *;
using FNodeListPtr = graph::NodeList<assem::Instr> *;
using FGraph = graph::Graph<assem::Instr>;
using FGraphPtr = graph::Graph<assem::Instr> *;

class FlowGraphFactory {
public:
  explicit FlowGraphFactory(assem::InstrList *instr_list)
      : instr_list_(instr_list), flowgraph_(new FGraph()),
        label_map_(std::make_unique<tab::Table<temp::Label, FNode>>()) {}
  void AssemFlowGraph();
  FGraphPtr GetFlowGraph() { return flowgraph_; }

private:
  assem::InstrList *instr_list_;
  FGraphPtr flowgraph_;
  std::unique_ptr<tab::Table<temp::Label, FNode>> label_map_;
};

} // namespace fg

#endif