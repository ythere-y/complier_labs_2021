#include "tiger/liveness/liveness.h"
#include <algorithm>

extern frame::RegManager *reg_manager;

namespace live {

bool contains(temp::TempList *list, temp::Temp *temp) {
  std::list<temp::Temp *> tempList = list->GetList();
  std::list<temp::Temp *>::iterator iter =
      std::find(tempList.begin(), tempList.end(), temp);
  return (iter != tempList.end());
}

temp::TempList *Union(temp::TempList *lhs, temp::TempList *rhs) {
  if (lhs == nullptr && rhs == nullptr)
    return new temp::TempList();
  else if (lhs == nullptr)
    return rhs;
  else if (rhs == nullptr)
    return lhs;

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
  if (lhs == nullptr)
    return new temp::TempList();
  else if (rhs == nullptr)
    return lhs;
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
bool equal(temp::TempList *lhs, temp::TempList *rhs) {
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
  std::set<int> inner;
  for (auto it_right : get_rhs)
    inner.insert(it_right->Int());
  for (auto it_left : get_lhs) {
    if (inner.find(it_left->Int()) == inner.end()) {
      LLOG("something wrong\n");
      return false;
    }
  }
  return true;
}
bool equal(std::map<fg::FNodePtr, temp::TempList *> lhs,
           std::map<fg::FNodePtr, temp::TempList *> rhs) {

  LTAN;

  if (lhs.size() != rhs.size()) {
    return false;
    // LTAN;
  }
  // LTAN;
  for (const auto &item : lhs) {
    if (rhs.find(item.first) == rhs.end()) {
      return false;
    }
    // LTAN;
    if (!equal(item.second, rhs[item.first])) {
      // LTAN;
      return false;
    }
  }
  // LTAN;
  return true;
}

bool Equal(temp::TempList *left, temp::TempList *right) {
  if (left == nullptr && right == nullptr)
    return true;
  else if (left == nullptr && right != nullptr)
    return false;
  else if (left != nullptr && right == nullptr)
    return false;

  for (auto it_left : left->GetList())
    if (!contains(right, it_left))
      return false;
  for (auto it_right : right->GetList())
    if (!contains(left, it_right))
      return false;
  return true;
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

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */

  std::map<fg::FNodePtr, temp::TempList *> lastIn, lastOut;
  std::list<fg::FNodePtr> nodeList = this->flowgraph_->Nodes()->GetList();

  bool fixedpoint = false;
  while (!fixedpoint) {
    fixedpoint = true;
    lastIn = *(in_.get());
    lastOut = *(this->out_.get());
    int num_count = 1;
    for (auto node_it = nodeList.rbegin(); node_it != nodeList.rend();
         node_it++) {
      temp::TempList *defs = (*node_it)->NodeInfo()->Def();
      temp::TempList *uses = (*node_it)->NodeInfo()->Use();
      auto temp_in = Union(uses, Subtract((*(this->out_))[*node_it], defs));
      auto temp_out = new temp::TempList();

      auto old_in = (*(in_))[*node_it];
      auto old_out = (*(out_))[*node_it];

      std::list<fg::FNodePtr> succList = (*node_it)->Succ()->GetList();
      for (auto it_succ : (*node_it)->Succ()->GetList())
        temp_out = Union(temp_out, (*(this->in_))[it_succ]);

      (*(this->in_))[*node_it] = temp_in;
      (*(this->out_))[*node_it] = temp_out;

      if (!Equal(old_in, temp_in) || !Equal(old_out, temp_out)) {
        fixedpoint = false;
      }
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

  std::list<fg::FNodePtr> nodeList = this->flowgraph_->Nodes()->GetList();
  for (auto node_it = nodeList.begin(); node_it != nodeList.end(); node_it++) {
    temp::TempList *defs = (*node_it)->NodeInfo()->Def();
    temp::TempList *uses = (*node_it)->NodeInfo()->Use();
    if (typeid(**node_it) == typeid(assem::MoveInstr) && defs && uses) {
      // Move instruction would never have more than 1 src or dst
      INodePtr srcNode = GetNode(uses->NthTemp(0));
      INodePtr dstNode = GetNode(defs->NthTemp(0));
      this->live_graph_.moves->Prepend(srcNode, dstNode);
      std::list<temp::Temp *> outTempList =
          (*(this->out_))[*node_it]->GetList();
      for (auto outTemp_it = outTempList.begin();
           outTemp_it != outTempList.end(); outTemp_it++) {
        if (*outTemp_it == uses->NthTemp(0)) {
          // for move instruction, there's no need to add conflict edges for
          // src node
          continue;
        }
        INodePtr outNode = GetNode(*outTemp_it);
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
}

void LiveGraphFactory::Liveness() {
  LiveMap();
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
