#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */
  // TODO:这是啥
  tree::Exp *ToExp(tree::Exp *framePtr) const { return nullptr; }
};

class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const { return new tree::TempExp(reg); }
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  X64Frame(){};
  X64Frame(temp::Label *name, std::vector<bool> escapes);
  Access *allocLocal(bool escape);
  temp::Label get_name() { return this->label_; }
  std::vector<Access> get_formals() { return this->fromals; }
};
/* TODO: Put your lab5 code here */

X64Frame::X64Frame(temp::Label *name, std::vector<bool> escapes) {

  this->label_ = name;
  this->fromals = new std::vector<Access>();
  this->locals = new std::vector<Access>();
  this->view_shift = new tree::StmList(nullptr, nullptr);
  tree::StmList *view_tail = view_shift;
  this->s_offset = -WORDSIZE;

  int formal_offset = WORDSIZE;

  int count = 0;
  for (auto it_es = escapes.begin(); it_es != escapes.end(); it_escape++) {
    if ((*it_es)) {
      add_ac = InFrameAccess(count * formal_offset);
    } else {
      add_ac = InRegAccess(RegManager::GetRegister(count)); //用一个寄存器来存
    }
    fromals.push_back(add_ac);
    count++;
  }
}

Access *X64Frame::allocLocal(bool escape) {
  if (escape) {
    frame_size++;
    return new InFrameAccess(-RegManager::WordSize() * (this->frame_size));
  } else {
    return InRegAccess(RegManager::GetRegister(frame_size)); //创造一个新的
  }
}

} // namespace frame