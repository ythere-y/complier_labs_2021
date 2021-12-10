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
  this->fromals_ = new std::vector<Access *>();
  this->locals_ = new std::vector<Access *>();
  this->view_shift_ = new tree::StmList();
  tree::StmList *view_tail = view_shift_;
  this->s_offset_ = -reg_manager->WordSize();

  int formal_offset = reg_manager->WordSize();

  int count = 0;
  if (escapes) {
    for (auto it_es = escapes->begin(); it_es != escapes->end(); it_es++) {
      Access *add_ac;
      if ((*it_es)) {
        add_ac = new InFrameAccess(count * formal_offset);
      } else {
        add_ac =
            new InRegAccess(reg_manager->GetRegister(count)); //用一个寄存器来存
      }
      fromals_->push_back(add_ac);
      count++;
    }
  }
}

Access *X64Frame::allocLocal(bool escape) {
  if (escape) {
    frame_size_++;
    return new InFrameAccess(-reg_manager->WordSize() * (this->frame_size_));
  } else {
    return new InRegAccess(
        reg_manager->GetRegister(frame_size_)); //创造一个新的
  }
}

tree::Exp *externalCall(std::string s, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)),
                           args);
}
// 主要进行视角转移
tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *stm) {

  int num = 1;
  tree::Stm *viewshift = new tree::ExpStm(new tree::ConstExp(0));
  auto get_formals = frame->fromals_;
  auto it_formals = get_formals->begin();

  for (; it_formals != get_formals->end(); it_formals++) {
    if (reg_manager->ArgRegs()->NthTemp(num))
      viewshift = new tree::SeqStm(
          viewshift,
          new tree::MoveStm(
              (*it_formals)
                  ->ToExp(new tree::TempExp(reg_manager->FramePointer())),
              new tree::TempExp(reg_manager->ArgRegs()->NthTemp(num))));
  }
  return new tree::SeqStm(viewshift, stm);
  /*
    tree::StmList *static_list = frame->view_shift;
    auto get_stm = static_list->GetList();
    auto it_stm = get_stm.begin();
    tree::Stm *bind = nullptr;
    for (; it_stm != get_stm.end(); it_stm++) {
      bind = new tree::SeqStm((*it_stm), bind);
    }
    return bind;
  */
}

// 在函数结束后说明哪些寄存器仍需要使用
assem::InstrList *ProcEntryExit2(assem::InstrList *instr_list) {
  instr_list->Append(
      new assem::OperInstr("", nullptr, reg_manager->ReturnSink(), nullptr));
  return instr_list;
}
// 给函数增加前缀和后缀
assem::Proc *ProcEntryExit3(Frame *frame, assem::InstrList *instr_list) {

  static char instr[256];

  std::string prolog;
  sprintf(instr, ".set %s_framesize, %d\n", frame->label_->Name().c_str(),
          -frame->s_offset_);
  prolog = std::string(instr);
  sprintf(instr, "%s:\n", frame->label_->Name().c_str());
  prolog.append(std::string(instr));
  sprintf(instr, "\tsubq $%s_framesize, %%rsp\n",
          frame->label_->Name().c_str());
  prolog.append(std::string(instr));

  sprintf(instr, "\taddq $%s_framesize, %%rsp\n",
          frame->label_->Name().c_str());
  std::string epilog = std::string(instr);
  epilog.append(std::string("\tret\n"));
  return new assem::Proc(prolog, instr_list, epilog);
  /*
  std::string prolog = frame->label_->Name();
  prolog.append(std::string(":\n.set"));
  prolog.append(frame->label_->Name());
  prolog.append(std::string("_framesize,$0x10000\n"
                            "\tpushq %rcx\n"
                            "\tpushq %rbp\n"
                            "\tmovq %rsp, %rbp\n"
                            "\tsubq $0x10000, %rsp\n"));
  std::string epilog = "\taddq $0x10000,%rsp\n"
                       "\tpopq %rbp\n"
                       "\tpopq %rcx\n"
                       "\tret\n";
  return new assem::Proc(prolog, instr_list, epilog);
  */
}
Frame::Frame(temp::Label *name, std::vector<bool> *escapes) {
  fromals_ = new std::vector<Access *>(0);
  auto it_esc = escapes->begin();
  int num = 0;
  for (; it_esc != escapes->end(); it_esc++)
    if ((*it_esc)) {
      fromals_->push_back(new InFrameAccess(reg_manager->WordSize() * num));
      num++;
    } else
      fromals_->push_back(new InRegAccess(temp::TempFactory::NewTemp()));
}
} // namespace frame