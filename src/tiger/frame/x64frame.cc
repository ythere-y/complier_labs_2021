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
  tree::Exp *ToExp(tree::Exp *framePtr) const {
    return new tree::MemExp(framePtr, new tree::ConstExp(offset));
  }
};

class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const { return new tree::TempExp(reg); }
};

/* TODO: Put your lab5 code here */

X64Frame::X64Frame(temp::Label *name, std::vector<bool> *escapes) {
  this->label_ = name;
  this->fromals = std::vector<Access *>();
  this->locals = std::vector<Access *>();
  this->view_shift = new tree::StmList();
  tree::StmList *view_tail = view_shift;
  this->s_offset = -reg_manager->WordSize();

  int formal_offset = reg_manager->WordSize();

  int count = 0;
  for (auto it_es = escapes->begin(); it_es != escapes->end(); it_es++) {
    Access *add_ac;
    if ((*it_es)) {
      add_ac = new InFrameAccess(count * formal_offset);
    } else {
      add_ac =
          new InRegAccess(reg_manager->GetRegister(count)); //用一个寄存器来存
    }
    fromals.push_back(add_ac);
    count++;
  }
}

Access *X64Frame::allocLocal(bool escape) {
  if (escape) {
    frame_size++;
    return new InFrameAccess(-reg_manager->WordSize() * (this->frame_size));
  } else {
    return new InRegAccess(reg_manager->GetRegister(frame_size)); //创造一个新的
  }
}

} // namespace frame