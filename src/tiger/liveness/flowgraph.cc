#include "tiger/liveness/flowgraph.h"

#define SAME(type_a, type_b) typeid(*(type_a)) == typeid((type_b))

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  LOG("AssemFlowGraph begin\n");
  flowgraph_ = new graph::Graph<assem::Instr *>();
  auto get_instr = instr_list_->GetList();
  auto it_instr = get_instr.begin();

  FNodePtr cur_node = nullptr, prev_node = nullptr;
  // 遍历所有的instruction
  for (; it_instr != get_instr.end(); it_instr++) {
    // 对每一个instruction创建一个node
    cur_node = flowgraph_->NewNode(*it_instr);
    // 如果存在前面的点
    if (prev_node)
      flowgraph_->AddEdge(prev_node, cur_node);

    if (SAME(*it_instr, assem::LabelInstr))
      label_map_->Enter(((assem::LabelInstr *)(*it_instr))->label_, cur_node);

    if (SAME(*it_instr, assem::OperInstr) &&
        ((assem::OperInstr *)(*it_instr))->assem_.find("jmp") == 0) {
      prev_node = nullptr;
    } else {
      // 否则是第一个点
      prev_node = cur_node;
    }
  }
  // 将跳转的边添加上
  auto get_node = flowgraph_->Nodes()->GetList();
  auto it_node = get_node.begin();
  for (; it_node != get_node.end(); it_node++) { // 遍历初步生成的graph
    if (SAME((*it_node)->NodeInfo(), assem::OperInstr) &&
        SAME(((assem::OperInstr *)(*it_node)->NodeInfo())->jumps_)) {

      auto get_labels =
          ((assem::OperInstr *)(*it_node)->NodeInfo())->jumps_->labels_;
      auto it_label = get_labels->begin();
      for (; it_label != get_labels.end(); it_label++) {
        flowgraph_->AddEdge((*it_node), label_map_->Look(*it_label));
      }
    }
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const { /* TODO: Put your lab6 code here */
  return nullptr;
}

temp::TempList *MoveInstr::Def() const { /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *OperInstr::Def() const { /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *LabelInstr::Use() const { /* TODO: Put your lab6 code here */
  return nullptr;
}

temp::TempList *MoveInstr::Use() const { /* TODO: Put your lab6 code here */
  return src_;
}

temp::TempList *OperInstr::Use() const { /* TODO: Put your lab6 code here */
  return src_;
}
} // namespace assem
