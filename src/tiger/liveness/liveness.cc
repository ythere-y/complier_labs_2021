#include "tiger/liveness/liveness.h"
#include <algorithm>

extern frame::RegManager *reg_manager;

namespace live {

static bool debug_flag = true;

bool contains(temp::TempList *list, temp::Temp *temp) {
  std::list<temp::Temp *> tempList = list->GetList();
  for (auto it = tempList.begin(); it != tempList.end(); it++) {
    if ((*it)->Int() == temp->Int()) {
      return true;
    }
  }
  return false;
  // std::list<temp::Temp *>::iterator iter =
  //     std::find(tempList.begin(), tempList.end(), temp);
  // return (iter != tempList.end());
}

void show_info(temp::Temp *out) {}

static void display(LiveGraph live_graph, INodeListPtr list) {
  LOG("ready to diplay\n");
  FILE *test_out = fopen("graph.out", "a+");
  // auto test = list->GetList().begin();
  // (*test)->NodeInfo()->Int();

  live_graph.interf_graph->Show(test_out, list);
  fclose(test_out);
}
static void display(LiveGraph live_graph) {
  display(live_graph, live_graph.interf_graph->Nodes());
}

temp::TempList *Union(temp::TempList *lhs, temp::TempList *rhs) {
  std::list<temp::Temp *> leftList = lhs->GetList();
  std::list<temp::Temp *> rightList = rhs->GetList();
  temp::TempList *result = new temp::TempList();

  for (auto ltemp_it = leftList.begin(); ltemp_it != leftList.end();
       ltemp_it++) {
    result->Append(*ltemp_it);
  }
  for (auto rtemp_it = rightList.begin(); rtemp_it != rightList.end();
       rtemp_it++) {
    if (!contains(lhs, *rtemp_it)) {
      result->Append(*rtemp_it);
    }
  }
  return result;
}

temp::TempList *Subtract(temp::TempList *lhs, temp::TempList *rhs) {
  std::list<temp::Temp *> leftList = lhs->GetList();
  std::list<temp::Temp *> rightList = rhs->GetList();
  temp::TempList *result = new temp::TempList();
  for (auto ltemp_it = leftList.begin(); ltemp_it != leftList.end();
       ltemp_it++) {
    if (!contains(rhs, *ltemp_it)) {
      result->Append(*ltemp_it);
    }
  }
  return result;
}

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  assert(move_it != move_list_.end());
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

bool TempListEqual(temp::TempList *lhs, temp::TempList *rhs) {
  if (lhs == nullptr && rhs == nullptr)
    return true;
  else if (lhs == nullptr && rhs != nullptr)
    return false;
  else if (lhs != nullptr && rhs == nullptr)
    return false;

  auto get_lhs = lhs->GetList();
  auto get_rhs = rhs->GetList();
  if (get_lhs.size() != get_rhs.size())
    return false;
  for (auto it_left : get_lhs) {
    if (!contains(rhs, it_left))
      return false;
  }
  return true;
}

bool equal(std::map<fg::FNodePtr, temp::TempList *> lhs,
           std::map<fg::FNodePtr, temp::TempList *> rhs) {

  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (const auto &item : lhs) {
    if (rhs.find(item.first) == rhs.end()) {
      return false;
    }
    if (!TempListEqual(item.second, rhs[item.first])) {
      return false;
    }
  }
  return true;
}

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  std::map<fg::FNodePtr, temp::TempList *> lastIn, lastOut;
  std::list<fg::FNodePtr> nodeList = this->flowgraph_->Nodes()->GetList();

  for (auto node_it = nodeList.begin(); node_it != nodeList.end(); node_it++) {
    if (!(*(this->out_))[*node_it]) {
      (*(this->out_))[*node_it] = new temp::TempList();
    }
    if (!(*(this->in_))[*node_it]) {
      (*(this->in_))[*node_it] = new temp::TempList();
    }
  }
  while (true) {
    lastIn = *(this->in_);
    lastOut = *(this->out_);

    for (auto node_it = nodeList.rbegin(); node_it != nodeList.rend();
         node_it++) {
      temp::TempList *defs = (*node_it)->NodeInfo()->Def();
      temp::TempList *uses = (*node_it)->NodeInfo()->Use();

      (*(this->in_))[*node_it] =
          Union(uses, Subtract((*(this->out_))[*node_it], defs));
      // (*(this->out_))[*node_it] = nullptr;
      // (*(this->out_))[*node_it] = new temp::TempList ();
      std::list<fg::FNodePtr> succList = (*node_it)->Succ()->GetList();
      for (auto succ_it = succList.begin(); succ_it != succList.end();
           succ_it++) {
        (*(this->out_))[*node_it] =
            Union((*(this->out_))[*node_it], (*(this->in_))[*succ_it]);
      }
    }

    if (equal(lastIn, (*(this->in_))) && equal(lastOut, (*(this->out_)))) {
      break;
    }
  }
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */

  // add precolored confliction
  for (temp::Temp *temp1 : reg_manager->Registers()->GetList()) {
    // Δ note that %rsp was excluded for UNKNOWN reason!
    for (temp::Temp *temp2 : reg_manager->Registers()->GetList()) {
      INodePtr temp1Node = GetNode(temp1);
      INodePtr temp2Node = GetNode(temp2);
      if (temp1Node != temp2Node) {
        this->live_graph_.interf_graph->AddEdge(temp1Node, temp2Node);
        this->live_graph_.interf_graph->AddEdge(temp2Node, temp1Node);
      }
    }
  }
  // display(live_graph_, live_graph_.interf_graph->Nodes());

  std::list<fg::FNodePtr> nodeList = this->flowgraph_->Nodes()->GetList();
  int move_count = 0;
  for (auto node_it = nodeList.rbegin(); node_it != nodeList.rend();
       node_it++) {
    temp::TempList *defs = (*node_it)->NodeInfo()->Def();
    temp::TempList *uses = (*node_it)->NodeInfo()->Use();
    if (typeid(*((*node_it)->NodeInfo())) == typeid(assem::MoveInstr)) {
      move_count++;
    }

    if (typeid(*((*node_it)->NodeInfo())) == typeid(assem::MoveInstr) &&
        !defs->GetList().empty() && !uses->GetList().empty()) {
      // Move instruction would never have more than 1 src or dst
      INodePtr srcNode = GetNode(uses->NthTemp(0));
      INodePtr dstNode = GetNode(defs->NthTemp(0));
      this->live_graph_.moves->Prepend(srcNode, dstNode);
      auto outTempList = (*(this->out_))[*node_it];
      LOG("get a move [%d -> %d]\n", (*(defs->GetList().begin()))->Int(),
          (*(uses->GetList().begin()))->Int());

      for (auto it_out : Subtract(outTempList, uses)->GetList()) {
        INodePtr outNode = GetNode(it_out);
        if (dstNode != outNode) {
          this->live_graph_.interf_graph->AddEdge(dstNode, outNode);
          this->live_graph_.interf_graph->AddEdge(outNode, dstNode);
        }
      }
    } else {
      std::list<temp::Temp *> defList = defs->GetList();
      for (auto def_it = defList.begin(); def_it != defList.end(); def_it++) {
        std::list<temp::Temp *> outTempList =
            (*(this->out_))[*node_it]->GetList();
        for (auto outTemp_it = outTempList.begin();
             outTemp_it != outTempList.end(); outTemp_it++) {
          INodePtr dstNode = GetNode(*def_it);
          INodePtr outNode = GetNode(*outTemp_it);
          if (dstNode != outNode) {
            this->live_graph_.interf_graph->AddEdge(dstNode, outNode);
            this->live_graph_.interf_graph->AddEdge(outNode, dstNode);
          }
        }
      }
    }
  }
  LOG("in total move count [count = %d]\n", move_count);
  display(live_graph_, live_graph_.interf_graph->Nodes());
}

void LiveGraphFactory::Liveness() {
  if (debug_flag) {
    printf("LiveMap begin!\n");
  }
  LiveMap();
  if (debug_flag) {
    printf("InterfGraph begin!\n");
  }
  InterfGraph();
}

INodePtr LiveGraphFactory::GetNode(temp::Temp *temp) {
  if (!this->temp_node_map_->Look(temp)) {
    this->temp_node_map_->Enter(temp,
                                this->live_graph_.interf_graph->NewNode(temp));
  }
  return this->temp_node_map_->Look(temp);
}

} // namespace live
