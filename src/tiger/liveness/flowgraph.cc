#include "tiger/liveness/flowgraph.h"
#include <map>

extern frame::RegManager *reg_manager;
namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  std::map<assem::Instr *, FNodePtr> instr2Node;

  std::list<assem::Instr *> instrList = this->instr_list_->GetList();
  FNodePtr curNode = nullptr, prevNode = nullptr;
  for (auto it = instrList.begin(); it != instrList.end(); it++) {
    curNode = this->flowgraph_->NewNode(*it);
    instr2Node[*it] = curNode;

    if (prevNode) {
      this->flowgraph_->AddEdge(prevNode, curNode);
    }
    if (typeid(**it) == typeid(assem::LabelInstr)) {
      this->label_map_->Enter(((assem::LabelInstr *)(*it))->label_, curNode);
    }
    if (typeid(**it) == typeid(assem::OperInstr) &&
        ((assem::OperInstr *)(*it))->assem_.find("jmp") == 0) {
      prevNode = nullptr;
    } else {
      prevNode = curNode;
    }
  }

  for (auto instr_it = instrList.begin(); instr_it != instrList.end();
       instr_it++) {
    if (typeid(**instr_it) == typeid(assem::OperInstr) &&
        ((assem::OperInstr *)(*instr_it))->jumps_) {
      std::vector<temp::Label *> *targetLabels =
          ((assem::OperInstr *)(*instr_it))->jumps_->labels_;
      for (auto label_it = targetLabels->begin();
           label_it != targetLabels->end(); label_it++) {
        assert(this->label_map_->Look(*label_it));
        assert(instr2Node.find(*instr_it) != instr2Node.end());
        FNodePtr jmpNode = instr2Node[*instr_it];
        FNodePtr labelNode = this->label_map_->Look(*label_it);
        this->flowgraph_->AddEdge(jmpNode, labelNode);
      }
    }
  }
}

} // namespace fg

namespace assem {

temp::TempList *filterRSP(temp::TempList *list) {
  temp::TempList *retList = new temp::TempList();

  std::list<temp::Temp *> tempList = list->GetList();

  for(auto it=tempList.begin(); it!=tempList.end(); it++) {
    if((*it)->Int() == reg_manager->StackPointer()->Int()){
      continue;
    } else {
      retList->Append(*it);
    }
  }
  return retList;
}

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  // return nullptr;
  return new temp::TempList();
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  // return this->dst_;  // \Δ should I filter the %rsp ?
  if (!this->dst_) {
    return new temp::TempList();
  } else {
    return filterRSP(this->dst_);
  }
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  if (!this->dst_) {
    return new temp::TempList();
  } else {
    return filterRSP(this->dst_);
  }
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  // return nullptr;
  return new temp::TempList();
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  // return this->src_;
  if (!this->src_) {
    return new temp::TempList();
  } else {
    return filterRSP(this->src_);
  }
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  // return this->src_;
  if (!this->src_) {
    return new temp::TempList();
  } else {
    return filterRSP(this->src_);
  }
}
} // namespace assem
