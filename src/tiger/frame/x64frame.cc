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
    // 传入fp，结合自己的offset取到值
    return new tree::MemExp(new tree::ConstExp(offset), framePtr);
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
  int word_size = reg_manager->WordSize();

  this->label_ = name;
  this->fromals_ = new std::vector<Access *>();
  this->locals_ = new std::vector<Access *>();
  this->view_shift_ = new tree::StmList();
  this->s_offset_ = -reg_manager->WordSize();
  this->s_offset_ = -word_size;
  this->frame_num_ = 0;

  int count = 0;
  int formal_offset = word_size;

  if (escapes) {
    for (auto it_es = escapes->begin(); it_es != escapes->end(); it_es++) {
      Access *add_ac;
      // 需要存frame中
      if (*it_es) {
        if (count < 6) {
          // 确定在frame中的位置
          add_ac = new InFrameAccess(s_offset_);
          // 增加一手将这个位置的东西移过来的操作
          view_shift_->Append(new tree::MoveStm(
              new tree::MemExp(new tree::ConstExp(s_offset_),
                               new tree::TempExp(reg_manager->FramePointer())),
              new tree::TempExp(reg_manager->ArgRegs()->NthTemp(count))));

          s_offset_ -= word_size;
        } else {
          add_ac = new InFrameAccess(s_offset_);
          s_offset_ -= word_size;
        }
      } else {
        temp::Temp *reg = temp::TempFactory::NewTemp();
        if (count < 6) {
          // 增加一手从args的地方拿来参数的操作
          view_shift_->Append(new tree::MoveStm(
              new tree::TempExp(reg),
              new tree::TempExp(reg_manager->ArgRegs()->NthTemp(count))));

        } else {
          printf("Frame : args more than 6.\n");
        }
        add_ac = new InRegAccess(reg);
      }

      fromals_->push_back(add_ac);
      count++;
    }
  }
}
Access *X64Frame::allocLocal(bool escape) {

  Access *local;
  if (escape) {
    local = new InFrameAccess(s_offset_);
    s_offset_ -= reg_manager->WordSize();
  } else {
    // if(args_num )
    local = new InRegAccess(temp::TempFactory::NewTemp());
  }
  return local;
}
tree::Exp *externalCall(std::string s, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)),
                           args);
}
// 主要进行视角转移
tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *stm) {
  FLOG("get in\n");

  tree::StmList *static_list = frame->view_shift_;
  auto get_stm = static_list->GetList();
  auto it_stm = get_stm.begin();
  tree::Stm *bind = nullptr;
  if (it_stm != get_stm.end()) {
    bind = (*it_stm);
    it_stm++;
  }
  for (; it_stm != get_stm.end(); it_stm++) {
    bind = new tree::SeqStm((*it_stm), bind);
  }
  if (bind) {
    bind = new tree::SeqStm(bind, stm);
  } else
    bind = stm;

  return bind;
}

// 在函数结束后说明哪些寄存器仍需要使用
assem::InstrList *ProcEntryExit2(assem::InstrList *instr_list) {
  instr_list->Append(
      new assem::OperInstr("", nullptr, reg_manager->ReturnSink(), nullptr));
  FLOG("exit2 finished ~~\n");
  return instr_list;
}
// 给函数增加前缀和后缀
assem::Proc *ProcEntryExit3(Frame *frame, assem::InstrList *instr_list) {
  FLOG("get here\n");
  static char instr[256];

  std::string prolog;
  sprintf(instr, ".set %s_framesize, %d\n", frame->label_->Name().c_str(),
          -frame->s_offset_);
  prolog = std::string(instr);
  sprintf(instr, "%s:\n", frame->label_->Name().c_str());
  prolog.append(std::string(instr));
  // sprintf(instr, "\tsubq $%s_framesize, %%rsp\n",
  //         frame->label_->Name().c_str());
  sprintf(instr, "\tsubq $%d , %%rsp\n", -frame->s_offset_);
  prolog.append(std::string(instr));

  // sprintf(instr, "\taddq $%s_framesize, %%rsp\n",
  //         frame->label_->Name().c_str());
  sprintf(instr, "\taddq $%d, %%rsp\n\n", -frame->s_offset_);
  std::string epilog = std::string(instr);
  epilog.append(std::string("\tretq\n\n"));
  FLOG("exit 3 finished ~~~\n");
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