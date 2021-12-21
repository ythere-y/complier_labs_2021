#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"
#include <map>
#include <set>

namespace ra {

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result() {}
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
public:
  RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> assemInstr)
      : frame_(frame), assemInstr_(std::move(assemInstr)),
        result_(std::make_unique<Result>()) {

    // initial = live_graph_->interf_graph->Nodes()->GetList();
    workListMoves = new live::MoveList();
    activeMoves = new live::MoveList();
    coalescedMoves = new live::MoveList();
    constrainedMoves = new live::MoveList();
    coloring = temp::Map::Empty();
  }
  ~RegAllocator() {
    delete workListMoves;
    delete activeMoves;
    delete coalescedMoves;
    delete constrainedMoves;
    delete live_graph_;
  }
  void RegAlloc();
  std::unique_ptr<Result> TransferResult() { return std::move(result_); }

private:
  void Build();
  void MakeWorkList();
  void Simplify();
  void Coalesce();
  void Freeze();
  void SelectSpill();
  void AssignColors();
  void RewriteProgram();
  assem::InstrList *RemoveUnnecessary();

  bool precolored(temp::Temp *temp);
  void AddEdge(live::INodePtr u, live::INodePtr v);
  live::MoveList *GetMoveList(live::INodePtr node);
  bool MoveRelated(live::INodePtr node);
  void DecrementDegree(live::INodePtr node);
  void EnableMove(live::INodeListPtr nodes);
  live::INodeListPtr Adjacent(live::INodePtr node);
  live::INodePtr GetAlias(live::INodePtr node);
  void AddWorkList(live::INodePtr node);
  bool Conservative(live::INodeListPtr nodes);
  void Combine(live::INodePtr u, live::INodePtr v);
  bool OK_forAll(live::INodeListPtr nodes, live::INodePtr r);
  bool OK(live::INodePtr t, live::INodePtr r);
  live::MoveList *NodeMoves(live::INodePtr node);
  void FreezeMoves(live::INodePtr u);

  frame::Frame *frame_;
  std::unique_ptr<cg::AssemInstr> assemInstr_;
  std::unique_ptr<Result> result_;

  // std::list<live::INodePtr> initial;

  std::set<live::INodePtr> simplifyWorkList;
  std::set<live::INodePtr> freezeWorkList;
  std::set<live::INodePtr> spillWorkList;

  std::vector<live::INodePtr> selectStack;

  std::set<live::INodePtr> coalescedNodes;
  std::set<live::INodePtr> spilledNodes;
  std::set<live::INodePtr> coloredNodes;

  std::map<live::INodePtr, live::MoveList *> moveList;

  std::set<std::pair<live::INodePtr, live::INodePtr>> adjSet;
  std::map<live::INodePtr, std::set<live::INodePtr>> adjList;
  std::map<live::INodePtr, int> degree;

  std::map<live::INodePtr, live::INodePtr> alias;

  std::set<temp::Temp *> noSpillTemp;

  // members that need to be explicitly initialized

  // initialize in RegAlloc()
  fg::FGraphPtr cfg_;
  live::LiveGraph *live_graph_;

  // initialize in construction
  live::MoveList *workListMoves;
  live::MoveList *activeMoves;
  live::MoveList *coalescedMoves;
  live::MoveList *constrainedMoves;
  live::MoveList *frozenMoves;

  temp::Map *coloring;
};

} // namespace ra

#endif