#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

namespace live {
using NodeTempListTable = tab::Table<fg::FNodePtr, temp::TempList *>;
using TempNodeTable = tab::Table<temp::Temp *, graph::Node<temp::Temp>>;

bool contains(temp::TempList *list, temp::Temp *temp) {
  auto get_list = list->GetList();
  auto find_it = std::find(get_list.begin(), get_list.end(), temp);
  return (find_it != tempList.end());
}

bool Equal(temp::TempList *left, temp::TempList *right) {
  auto get_left = left->GetList();
  auto get_right = right->GetList();
  for (auto it_left : get_left)
    if (!contains(get_right, it_left))
      return false;
  for (auto it_right : get_right)
    if (!contains(get_left, it_right))
      return false;
  return true;
}

temp::TempList *Union(temp::TempList *lhs, temp::TempList *rhs) {
  auto get_left = lhs->GetList();
  auto get_right = rhs->GetList();
  temp::TempList *res = new temp::TempList();

  for (auto it_left : get_left)
    res->Append(it_left);
  for (auto it_right : get_right)
    if (!contains(lhs, it_right))
      res->Append(it_right);
  return res;
}

temp::TempList *Subtract(temp::TempList *lhs, temp::TempList *rhs) {
  auto get_left = lhs->GetList();
  auto get_right = rhs->GetList();
  temp::TempList *res = new temp::TempList();
  for (auto it_left : get_left)
    if (!contains(rhs, it_right))
      res->Append(it_left);
  return res;
}

void LiveGraphFactory::AddLine(temp::Temp *left, temp::Temp *right) {
  if (!temp_node_map_->Look(left))
    temp_node_map_->Enter(left, live_graph_.interf_graph->NewNode(left));
  auto get_left = temp_node_map_->Look(left);
  if (!temp_node_map_->Look(right))
    temp_node_map_->Enter(right, live_graph_.interf_graph->NewNode(right));
  auto get_right = temp_node_map_->Look(right);

  if (get_left != get_right) {
    live_graph_.interf_graph->AddEdge(get_left, get_right);
    live_graph_.interf_graph->AddEdge(get_right, get_left);
  }
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

void LiveGraphFactory::LiveMap() { /* TODO: Put your lab6 code here */
  LOG("Live Map begin\n");
  NodeTempListTable *in = new NodeTempListTable();
  NodeTempListTable *out = new NodeTempListTable();
  auto get_nodes = flowgraph_->Nodes()->GetList();

  int turn_num = 1;
  while (true) {
    LOG("one turn [num = %d]\n", turn_num);
    for (auto re_it_node = get_nodes.rbegin(); re_it_node != get_nodes.rend();
         re_it_node++) {
      auto old_in_list = in_->Look(*re_it_node);
      auto old_out_list = out_->Look(*re_it_node);
      auto defs = (*re_it_node)->NodeInfo()->Def()->GetList();
      auto uses = (*re_it_node)->NodeInfo()->Use()->GetList();
      temp::TempList *tmp_out = new temp::TempList();
      temp::TempList *tmp_in = new temp::TempList();
      for (auto succ : (*re_it_node)->Succ()->GetList()) {
        Union(tmp_out, in_->Look(succ));
      }
      tmp_in = Subtract(tmp_out, defs);

      out_->Set((*re_it_node), tmp_out);
      in_->Set((*re_it_node), tmp_in);

      if (Equal(old_in_list, tmp_in) && Equal(old_out_list, tmp_out))
        break;
    }
  }
}

void LiveGraphFactory::InterfGraph() { /* TODO: Put your lab6 code here */
  LOG("Interference Graph begin\n");
  // step 1 register interference
  for (auto it_reg_1 : reg_manager->Registers()->GetList()) {
    for (auto it_reg_2 : reg_manager->Registers()->GetList()) {
      AddLine(it_reg_1, it_reg_2);
    }
  }
  auto get_nodes = flowgraph_->Nodes()->GetList();
  for (auto node : get_nodes) {
    auto defs = node->NodeInfo()->Def();
    auto uses = node->NodeInfo()->Use();
    if (typeid(*node) != typeid(assem::MoveInstr)) {
      // 不是移动指令
      for (auto it_def : defs->GetList()) {
        auto out_list = out_->Look(node)->GetList();
        for (auto it_out_list : out_list) {
          // TODO:是否需要加入rsp的判断
          AddLine(it_def, it_out_list);
        }
      }
    } else {
      // 是移动指令,要删除src中的一个结点
      for (auto it_def : defs->GetList()) {
        auto out_list = Subtract(out_->Look(node), uses)->GetList();
        for (auto it_out_list : out_list) {
          AddLine(it_def, it_out_list);
        }
      }
    }
  }
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live
