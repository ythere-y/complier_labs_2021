#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"
#include <sstream>

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */

// static void display(live::LiveGraph live_graph, live::INodeListPtr list) {
//   LOG("ready to diplay\n");
//   FILE *test_out = fopen("graph.out", "a+");
//   // auto test = list->GetList().begin();
//   // (*test)->NodeInfo()->Int();

//   live_graph.interf_graph->Show(test_out, list);
//   fclose(test_out);
// }

static bool test_mode = true;

void RegAllocator::RegAlloc() {
  fg::FlowGraphFactory flow_graph_factory(this->assemInstr_->GetInstrList());
  flow_graph_factory.AssemFlowGraph();
  cfg_ = flow_graph_factory.GetFlowGraph();

  live::LiveGraphFactory live_graph_factory(cfg_);
  live_graph_factory.Liveness();
  // live_graph_ = &(live_graph_factory.GetLiveGraph());

  live_graph_ = new live::LiveGraph(live_graph_factory.GetLiveGraph());

  // display(live_graph_, live_graph_->interf_graph->Nodes());

  RLOG("Build begin\n");
  Build();
  RLOG("Build finish\n");

  MakeWorkList();
  if (test_mode) {
    printf("MakeWorkList end!\n");
  }
  do {
    if (!simplifyWorkList.empty()) {
      RLOG("Simplify begin\n");
      Simplify();
      RLOG("Simplify finish\n");
    } else if (!workListMoves->GetList().empty()) {
      RLOG("Coalesce begin\n");
      Coalesce();
      RLOG("Coalesce begin\n");
    } else if (!freezeWorkList.empty()) {
      RLOG("Freeze begin\n");
      Freeze();
      RLOG("Freeze begin\n");
    } else if (!spillWorkList.empty()) {
      SelectSpill();
    }
  } while (!simplifyWorkList.empty() || !workListMoves->GetList().empty() ||
           !freezeWorkList.empty() || !spillWorkList.empty());
  AssignColors();
  if (!spilledNodes.empty()) {
    RewriteProgram();
    RegAlloc();
  } else {
    result_->coloring_ = coloring;
    // this->assemInstr_->GetInstrList()
    result_->il_ = RemoveUnnecessary();
  }
}

bool RegAllocator::precolored(temp::Temp *temp) {
  auto regs = reg_manager->Registers()->GetList();
  auto fi = std::find(regs.begin(), regs.end(), temp);
  if (fi != regs.end()) {
    return true;
  } else
    return false;
}

void RegAllocator::AddEdge(live::INodePtr u, live::INodePtr v) {
  if (adjSet.find(std::make_pair(u, v)) == adjSet.end() &&
      u->Key() != v->Key()) {
    adjSet.insert(std::make_pair(u, v));
    adjSet.insert(std::make_pair(v, u));
    if (!precolored((u)->NodeInfo())) {
      adjList[u].insert(v);
      degree[u]++;
    }
    if (!precolored((v)->NodeInfo())) {
      adjList[v].insert(u);
      degree[v]++;
    }
  }
}

//Δ quite different from text book
void RegAllocator::Build() {

  std::list<std::pair<live::INodePtr, live::INodePtr>> _moveList =
      live_graph_->moves->GetList();
  // 遍历所有move，记录记录每一条move，存在map中，方便用点来找到所有的move
  for (auto move_it = _moveList.begin(); move_it != _moveList.end();
       move_it++) {
    if (moveList.find((*move_it).first) == moveList.end()) {
      moveList[(*move_it).first] = new live::MoveList();
    }
    if (moveList.find((*move_it).second) == moveList.end()) {
      moveList[(*move_it).second] = new live::MoveList();
    }
    moveList[(*move_it).first]->Append((*move_it).first, (*move_it).second);
    moveList[(*move_it).second]->Append((*move_it).first, (*move_it).second);
  }

  workListMoves = this->live_graph_->moves; // 存储所有的move操作的list

  std::list<live::INodePtr> interf_graph_nodeList =
      live_graph_->interf_graph->Nodes()->GetList();
  // 遍历所有的结点，根据关系建立冲突图
  for (auto node_it = interf_graph_nodeList.begin();
       node_it != interf_graph_nodeList.end(); node_it++) {
    std::list<live::INodePtr> adjNodeList = (*node_it)->Adj()->GetList();
    for (auto adj_node_it = adjNodeList.begin();
         adj_node_it != adjNodeList.end(); adj_node_it++) {
      AddEdge(*node_it, *adj_node_it);
    }
  }

  temp::TempList *regList = reg_manager->Registers();
  for (int i = 0; i < frame::X64Frame::K; i++) {
    temp::Temp *ithRegister = regList->NthTemp(i);
    std::string *color = new std::string;
    coloring->Enter(ithRegister, reg_manager->temp_map_->Look(ithRegister));
  }

  coloring->Enter(reg_manager->StackPointer(), new std::string("%rsp"));
}

live::MoveList *RegAllocator::NodeMoves(live::INodePtr node) {
  return moveList[node]->Intersect(activeMoves->Union(workListMoves));
}

bool RegAllocator::MoveRelated(live::INodePtr node) {
  return !NodeMoves(node)->GetList().empty();
}

void RegAllocator::MakeWorkList() {
  std::list<live::INodePtr> interf_nodes =
      live_graph_->interf_graph->Nodes()->GetList();
  for (auto node_it = interf_nodes.begin(); node_it != interf_nodes.end();
       node_it++) {
    temp::Temp *temp = (*node_it)->NodeInfo();
    if (precolored(temp)) {
      continue;
    }
    if (degree[*node_it] >= frame::X64Frame::K) {
      spillWorkList.insert(*node_it);
    } else if (MoveRelated(*node_it)) {
      freezeWorkList.insert(*node_it);
    } else {
      simplifyWorkList.insert(*node_it);
    }
  }
  RLOG("spill work list [size = %d]\n", spillWorkList.size());
  RLOG("freeze work list [size = %d]\n", freezeWorkList.size());
  RLOG("simple work list [size = %d]\n", simplifyWorkList.size());
}

void RegAllocator::EnableMove(live::INodeListPtr nodes) {
  std::list<live::INodePtr> nodeList = nodes->GetList();
  for (auto node_it = nodeList.begin(); node_it != nodeList.end(); node_it++) {
    std::list<std::pair<live::INodePtr, live::INodePtr>> move_list =
        NodeMoves(*node_it)->GetList();
    for (auto move_it = move_list.begin(); move_it != move_list.end();
         move_it++) {
      if (!activeMoves->GetList().empty() &&
          activeMoves->Contain(move_it->first, move_it->second)) {
        activeMoves->Delete(move_it->first, move_it->second);
        if (!workListMoves->Contain(move_it->first, move_it->second))
          workListMoves->Append(move_it->first, move_it->second);
      }
    }
  }
}

live::INodeListPtr RegAllocator::Adjacent(live::INodePtr node) {
  live::INodeListPtr result = new live::INodeList();
  for (live::INodePtr adj : adjList[node]) {
    if (std::find(selectStack.begin(), selectStack.end(), adj) ==
            selectStack.end() &&
        coalescedNodes.find(adj) == coalescedNodes.end()) {
      result->Prepend(adj);
    }
  }
  return result;
}

void RegAllocator::DecrementDegree(live::INodePtr node) {
  // 如果是已经着色的，那么直接返回？
  // if (precolored(node->NodeInfo())) {
  //   return;
  // }

  int d = degree[node];
  degree[node] = d - 1;
  if (d == frame::X64Frame::K) {
    live::INodeListPtr adjNodes = Adjacent(node);
    adjNodes->Prepend(node);
    EnableMove(adjNodes);
    spillWorkList.erase(node);
    if (MoveRelated(node)) {
      freezeWorkList.insert(node);
    } else {
      simplifyWorkList.insert(node);
    }
  }
}

void RegAllocator::Simplify() {
  live::INodePtr node = *(simplifyWorkList.begin());
  auto cho_node = simplifyWorkList.begin();
  int sim_size = simplifyWorkList.size();
  int sel_size = selectStack.size();
  int no_num = node->NodeInfo()->Int();
  RLOG("[num = %d] sim [size = %d -> %d]__ sel [size = %d -> %d]\n", no_num,
       sim_size, sim_size - 1, sel_size, sel_size + 1);
  simplifyWorkList.erase(cho_node);

  selectStack.push_back(node);

  //Δ not same with textbook

  // this version
  // std::list<live::INodePtr> adjNodeList = node->Adj()->GetList();

  // textbook version
  std::list<live::INodePtr> adjNodeList = Adjacent(node)->GetList();
  R_ONELONGLOG("neighbor node info :");
  for (auto adj_node_it = adjNodeList.begin(); adj_node_it != adjNodeList.end();
       adj_node_it++) {
    R_ONELONGLOG("%d(%d), ", (*adj_node_it)->NodeInfo()->Int(),
                 degree[(*adj_node_it)] - 1);
    DecrementDegree(*adj_node_it);
  }
  R_ONELONGLOG("\n");
}

live::INodePtr RegAllocator::GetAlias(live::INodePtr node) {
  if (coalescedNodes.find(node) != coalescedNodes.end()) {
    return GetAlias(alias[node]);
  } else {
    return node;
  }
}

void RegAllocator::AddWorkList(live::INodePtr node) {
  if (!precolored(node->NodeInfo()) && !MoveRelated(node) &&
      degree[node] < frame::X64Frame::K) {
    freezeWorkList.erase(node);
    simplifyWorkList.insert(node);
  }
}

bool RegAllocator::Conservative(live::INodeListPtr nodes) {
  int k = 0;
  std::list<live::INodePtr> nodeList = nodes->GetList();
  for (auto node_it = nodeList.begin(); node_it != nodeList.end(); node_it++) {
    if (precolored((*node_it)->NodeInfo()) ||
        degree[*node_it] >= frame::X64Frame::K) {
      k++;
    }
  }
  return k < frame::X64Frame::K;
}

void RegAllocator::Combine(live::INodePtr u, live::INodePtr v) {
  //  printf("Combine: %d and %d\n", u->NodeInfo()->Int(),
  //  v->NodeInfo()->Int());
  if (freezeWorkList.find(v) != freezeWorkList.end()) {
    freezeWorkList.erase(v);
  } else {
    spillWorkList.erase(v);
  }
  coalescedNodes.insert(v);
  alias[v] = u;
  moveList[u] = moveList[u]->Union(moveList[v]);

  live::INodeListPtr nodeList = new live::INodeList();
  nodeList->Append(v);

  EnableMove(nodeList);

  std::list<live::INodePtr> adjList = Adjacent(v)->GetList();
  for (auto adj_it = adjList.begin(); adj_it != adjList.end(); adj_it++) {
    AddEdge(*adj_it, u);
    DecrementDegree(*adj_it);
  }

  if (degree[u] >= frame::X64Frame::K &&
      freezeWorkList.find(u) != freezeWorkList.end()) {
    freezeWorkList.erase(u);
    spillWorkList.insert(u);
  }
}

bool RegAllocator::OK(live::INodePtr t, live::INodePtr r) {
  return degree[t] < frame::X64Frame::K || precolored(t->NodeInfo()) ||
         adjSet.find(std::make_pair(t, r)) != adjSet.end();
}

bool RegAllocator::OK_forAll(live::INodeListPtr nodes, live::INodePtr r) {
  std::list<live::INodePtr> nodeList = nodes->GetList();
  if (!nodes || nodeList.empty()) {
    return true;
  }
  for (auto node_it = nodeList.begin(); node_it != nodeList.end(); node_it++) {
    if (!OK(*node_it, r)) {
      return false;
    }
  }
  return true;
}

void RegAllocator::Coalesce() {
  live::INodePtr x = workListMoves->GetList().begin()->first;  // src
  live::INodePtr y = workListMoves->GetList().begin()->second; // dst
  live::INodePtr u, v;

  // workListMoves->GetList().erase(workListMoves->GetList().begin());

  x = GetAlias(x);
  y = GetAlias(y);

  if (precolored(y->NodeInfo())) { //Δ
    u = y;
    v = x;
  } else {
    u = x;
    v = y;
  }
  workListMoves->Delete(x, y);
  if (u == v) {
    coalescedMoves->Append(x, y);
    AddWorkList(u);
  } else if (precolored(v->NodeInfo()) ||
             adjSet.find(std::make_pair(u, v)) != adjSet.end()) {
    if (!constrainedMoves->Contain(x, y))
      constrainedMoves->Append(x, y);
    AddWorkList(u);
    AddWorkList(v);
  } else {
    live::INodeListPtr adjNodes = Adjacent(u);

    //Δ not same with textbook
    // this version
    // adjNodes->CatList(Adjacent(v));

    // textbook version
    adjNodes->Union(Adjacent(v));
    if ((precolored(u->NodeInfo()) && OK_forAll(Adjacent(v), u)) ||
        (!precolored(u->NodeInfo()) && Conservative(adjNodes))) {
      coalescedMoves->Append(x, y);
      Combine(u, v);
      AddWorkList(u);
    } else {
      if (!activeMoves->Contain(x, y))
        activeMoves->Append(x, y);
    }
  }
}

void RegAllocator::FreezeMoves(live::INodePtr u) {
  std::list<std::pair<live::INodePtr, live::INodePtr>> nodeList =
      NodeMoves(u)->GetList();
  for (auto node_it = nodeList.begin(); node_it != nodeList.end(); node_it++) {
    live::INodePtr v;
    auto x = node_it->first;
    auto y = node_it->second;
    if (GetAlias(y) == GetAlias(u)) {
      v = GetAlias(x);
    } else {
      v = GetAlias(y);
    }
    if (activeMoves->Contain(x, y))
      activeMoves->Delete(x, y);

    if (!frozenMoves->Contain(x, y))
      frozenMoves->Append(x, y);

    if (
        //Δ different, the precolor condition is not from textbook but it is
        // probably needed
        !precolored(v->NodeInfo()) && NodeMoves(v)->GetList().empty() &&
        degree[v] < frame::X64Frame::K) {
      freezeWorkList.erase(v);
      simplifyWorkList.insert(v);
    }
  }
}

void RegAllocator::Freeze() {
  auto node = *(freezeWorkList.begin());
  freezeWorkList.erase(node);
  simplifyWorkList.insert(node);
  FreezeMoves(node);
}

void RegAllocator::SelectSpill() {
  live::INodePtr chosen = nullptr;
  double chosen_priority = 1E20;
  for (auto node : spillWorkList) {

    // Δ noSpillTemp is not in textbook
    if (noSpillTemp.find(node->NodeInfo()) != noSpillTemp.end()) {
      continue;
    }
    chosen = node;
    break;
    // if (liveness.priority[node->NodeInfo()] < chosen_priority) {
    //   chosen = node;
    //   chosen_priority = liveness.priority[node->NodeInfo()];
    // }
  }
  if (!chosen) {
    chosen = *(spilledNodes.begin());
  }
  spillWorkList.erase(chosen);
  simplifyWorkList.insert(chosen);
  FreezeMoves(chosen);
}

void RegAllocator::AssignColors() {
  // todo:
  while (!selectStack.empty()) {
    auto n = selectStack[selectStack.size() - 1];
    selectStack.pop_back();

    // todo
    // std::set<std::string> okColors;
    std::set<std::string> okColors;
    std::list<temp::Temp *> registers = reg_manager->Registers()->GetList();
    std::list<std::string> colors;
    for (auto reg_it = registers.begin(); reg_it != registers.end(); reg_it++) {
      colors.push_back(*(reg_manager->temp_map_->Look(*reg_it)));
    }

    okColors.insert(colors.begin(), colors.end());
    for (auto w : adjList[n]) {
      if (coloredNodes.find(GetAlias(w)) != coloredNodes.end() ||
          precolored(GetAlias(w)->NodeInfo())) {
        okColors.erase(*(coloring->Look(GetAlias(w)->NodeInfo())));
      }
    }
    if (okColors.empty()) {
      spilledNodes.insert(n);
    } else {
      coloredNodes.insert(n);
      std::string *color = new std::string(*(okColors.begin()));
      // std::string *color = new std::string();
      // *color = *(okColors.begin());
      coloring->Enter(n->NodeInfo(), color);
    }
  }
  for (auto n : coalescedNodes) {
    coloring->Enter(n->NodeInfo(), coloring->Look(GetAlias(n)->NodeInfo()));
  }
}

void RegAllocator::RewriteProgram() {
  std::list<assem::Instr *> newInstrList;

  for (auto node : spilledNodes) {
    temp::Temp *spilledTemp = node->NodeInfo();
    this->frame_->offset -= reg_manager->WordSize(); //Δ
    std::list<assem::Instr *> instrList =
        this->assemInstr_->GetInstrList()->GetList();

    if (!newInstrList.empty()) {
      instrList = newInstrList;
      this->assemInstr_->GetInstrList()->UpdateList(newInstrList);
      newInstrList.clear();
    }

    for (auto il_it = instrList.cbegin(); il_it != instrList.cend(); il_it++) {
      temp::TempList *src, *dst;
      if (typeid(**il_it) == typeid(assem::LabelInstr)) {
        src = new temp::TempList();
        dst = new temp::TempList();
      } else if (typeid(**il_it) == typeid(assem::MoveInstr)) {
        auto moveInstr = (assem::MoveInstr *)(*il_it);
        if (!moveInstr->src_ || moveInstr->src_->GetList().empty()) {
          src = new temp::TempList();
        } else {
          src = moveInstr->src_;
        }
        if (!moveInstr->dst_ || moveInstr->dst_->GetList().empty()) {
          dst = new temp::TempList();
        } else {
          dst = moveInstr->dst_;
        }
      } else if (typeid(**il_it) == typeid(assem::OperInstr)) {
        auto moveInstr = (assem::OperInstr *)(*il_it);
        if (!moveInstr->src_ || moveInstr->src_->GetList().empty()) {
          src = new temp::TempList();
        } else {
          src = moveInstr->src_;
        }
        if (!moveInstr->dst_ || moveInstr->dst_->GetList().empty()) {
          dst = new temp::TempList();
        } else {
          dst = moveInstr->dst_;
        }
      } else {
        assert(false);
      }

      assert(src && dst);
      std::list<temp::Temp *> srcTempList = src->GetList();
      std::list<temp::Temp *> dstTempList = dst->GetList();

      if (std::find(srcTempList.begin(), srcTempList.end(), spilledTemp) !=
              srcTempList.end() &&
          std::find(dstTempList.begin(), dstTempList.end(), spilledTemp) !=
              dstTempList.end()) {
        temp::Temp *newTemp = temp::TempFactory::NewTemp();
        noSpillTemp.insert(newTemp);
        src->Replace(spilledTemp, newTemp);
        dst->Replace(spilledTemp, newTemp);
        std::stringstream stream;
        stream << "movq (" << this->frame_->frame_size_ + this->frame_->offset
               << ")(`s0), `d0";
        std::string assem = stream.str();
        // this->assemInstr_->GetInstrList()->Insert(
        //     il_it,
        //     new assem::OperInstr(
        //         assem, new temp::TempList({newTemp}),
        //         new temp::TempList(reg_manager->StackPointer()), nullptr));

        newInstrList.push_back(new assem::OperInstr(
            assem, new temp::TempList({newTemp}),
            new temp::TempList(reg_manager->StackPointer()), nullptr));

        newInstrList.push_back(*il_it);

        stream.str(0);
        stream << "movq `s0, ("
               << this->frame_->frame_size_ + this->frame_->offset << ")(`s1)";
        assem = stream.str();
        // this->assemInstr_->GetInstrList()->Insert(
        //     ++il_it,
        //     new assem::OperInstr(
        //         assem, nullptr,
        //         new temp::TempList({newTemp, reg_manager->StackPointer()}),
        //         nullptr));
        // --il_it;
        newInstrList.push_back(new assem::OperInstr(
            assem, nullptr,
            new temp::TempList({newTemp, reg_manager->StackPointer()}),
            nullptr));
      } else if (std::find(srcTempList.begin(), srcTempList.end(),
                           spilledTemp) != srcTempList.end()) {
        temp::Temp *newTemp = temp::TempFactory::NewTemp();
        noSpillTemp.insert(newTemp);
        src->Replace(spilledTemp, newTemp);
        std::stringstream stream;
        stream << "movq (" << this->frame_->frame_size_ + this->frame_->offset
               << ")(`s0), `d0";
        std::string assem = stream.str();
        // this->assemInstr_->GetInstrList()->Insert(
        //     il_it,
        //     new assem::OperInstr(
        //         assem, new temp::TempList({newTemp}),
        //         new temp::TempList(reg_manager->StackPointer()), nullptr));

        newInstrList.push_back(new assem::OperInstr(
            assem, new temp::TempList({newTemp}),
            new temp::TempList(reg_manager->StackPointer()), nullptr));
        newInstrList.push_back(*il_it);
      } else if (std::find(dstTempList.begin(), dstTempList.end(),
                           spilledTemp) != dstTempList.end()) {
        temp::Temp *newTemp = temp::TempFactory::NewTemp();
        noSpillTemp.insert(newTemp);
        dst->Replace(spilledTemp, newTemp);
        std::stringstream stream;
        stream << "movq `s0, ("
               << this->frame_->frame_size_ + this->frame_->offset << ")(`s1)";
        std::string assem = stream.str();
        // this->assemInstr_->GetInstrList()->Insert(
        //     ++il_it,
        //     new assem::OperInstr(
        //         assem, nullptr,
        //         new temp::TempList({newTemp, reg_manager->StackPointer()}),
        //         nullptr));
        // --il_it;

        newInstrList.push_back(*il_it);
        newInstrList.push_back(new assem::OperInstr(
            assem, nullptr,
            new temp::TempList({newTemp, reg_manager->StackPointer()}),
            nullptr));
      } else {
        newInstrList.push_back(*il_it);
      }
    }
  }
  if (test_mode) {
    printf("loop finish\n");
  }
  spilledNodes.clear();
  coloredNodes.clear();
  coalescedNodes.clear();
}

assem::InstrList *RegAllocator::RemoveUnnecessary() {
  assem::InstrList *newInstrList = new assem::InstrList();
  std::list<assem::Instr *> instrList =
      this->assemInstr_->GetInstrList()->GetList();
  for (auto il_it = instrList.begin(); il_it != instrList.end(); il_it++) {
    if (typeid(**il_it) == typeid(assem::MoveInstr)) {
      auto *moveInstr = (assem::MoveInstr *)(*il_it);
      temp::Temp *src = moveInstr->src_->NthTemp(0),
                 *dst = moveInstr->dst_->NthTemp(0);
      if (!coloring->Look(src)->compare(*coloring->Look(dst))) {
        continue;
      }
    }
    newInstrList->Append(*il_it);
  }
  this->assemInstr_->GetInstrList()->UpdateList(newInstrList->GetList());
  return newInstrList;
}
} // namespace ra