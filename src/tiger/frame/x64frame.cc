#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */
};

class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
};

/* TODO: Put your lab5 code here */

Access *X64Frame::allocLocal(bool escape) {
  frame::Access *access;
  if (escape) {
    access = new InFrameAccess(offset);
    offset -= reg_manager->WordSize();
    this->frame_size_ += reg_manager->WordSize();
    // this->locals.push_back(access);
  } else {
    access = new InRegAccess(temp::TempFactory::NewTemp());
  }
  locals.push_back(access);
  return access;
}

tree::Exp *X64Frame::exp(frame::Access *access, tree::Exp *framePtr) {
  if (typeid(*access) == typeid(InFrameAccess)) {
    return new tree::MemExp(new tree::BinopExp(
        tree::PLUS_OP, framePtr,
        new tree::ConstExp(((InFrameAccess *)access)->offset)));
  } else {
    return new tree::TempExp(((frame::InRegAccess *)access)->reg);
  }
}

tree::Exp *X64Frame::externalCall(const std::string &s, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)),
                           args);
}

Frame *X64Frame::newFrame(temp::Label *name, std::vector<bool> *_boolList) {
  Frame *frame = new X64Frame();
  frame->name_ = name;
  frame->formals = std::list<frame::Access *>();
  frame->locals = std::list<frame::Access *>();
  frame->offset = -reg_manager->WordSize();
  frame->frame_size_ = 0;
  frame->viewShift = new tree::StmList();

  if (!_boolList) {
    return frame;
  }
  std::vector<bool> boolList = *_boolList;

  for (auto bool_it = boolList.begin(); bool_it != boolList.end(); bool_it++) {
    frame::Access *access;
    if ((*bool_it)) {
      frame->frame_size_ += reg_manager->WordSize();
      access = new InFrameAccess(frame->offset);
      frame->offset -= reg_manager->WordSize();
    } else {
      access = new InRegAccess(temp::TempFactory::NewTemp());
    }
    frame->formals.push_back(access);
  }

  int i = 0;
  for (auto formal_it = frame->formals.begin();
       formal_it != frame->formals.end(); formal_it++, i++) {
    tree::Exp *dstExp;
    if (typeid(*(*formal_it)) == typeid(frame::InFrameAccess)) {
      dstExp = new tree::MemExp(new tree::BinopExp(
          tree::PLUS_OP,
          new tree::BinopExp(tree::BinOp::PLUS_OP,
                             new tree::TempExp(reg_manager->StackPointer()),
                             new tree::TempExp(reg_manager->GetRegister(1))),
          new tree::ConstExp(((frame::InFrameAccess *)(*formal_it))->offset)));

    } else {
      dstExp = new tree::TempExp(((frame::InRegAccess *)(*formal_it))->reg);
    }
    tree::Stm *stm;
    if (i < 6) {
      stm = new tree::MoveStm(
          dstExp, new tree::TempExp(reg_manager->ArgRegs()->NthTemp(i)));
    } else {
      frame->frame_size_ += reg_manager->WordSize();
      stm = new tree::MoveStm(
          dstExp,
          new tree::MemExp(new tree::BinopExp(
              tree::BinOp::PLUS_OP,
              new tree::BinopExp(
                  tree::BinOp::PLUS_OP,
                  new tree::TempExp(reg_manager->StackPointer()),
                  new tree::TempExp(reg_manager->GetRegister(1))),
              new tree::ConstExp((i - 6 + 1) * reg_manager->WordSize()))));
    }
    frame->viewShift->Append(stm);
  }
  return frame;
}

static std::string reg_names[] = {
    "%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%rbp", "%rsp",
    "%r8",  "%r9",  "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};

X64RegManager::X64RegManager() {
  std::string *name;
  for (int i = 0; i < 16; i++) {
    regs_.push_back(temp::TempFactory::NewTemp());
  }
  for (int i = 0; i < 16; i++) {
    name = &(reg_names[i]);
    temp_map_->Enter(regs_[i], name);
  }

  // %rbs was used to store the frame size, so will no be return as general register.
  _registers = new temp::TempList({regs_[0], 
                                    regs_[1], 
                                    regs_[2], regs_[3],
                                   regs_[4], regs_[5], regs_[6], regs_[8],
                                   regs_[9], regs_[10], regs_[11], regs_[12],
                                   regs_[13], regs_[14], regs_[15]});

  _argRegs = new temp::TempList(
      {regs_[5], regs_[4], regs_[3], regs_[2], regs_[8], regs_[9]});

  _callerSaves =
      new temp::TempList({regs_[0], regs_[5], regs_[4], regs_[3], regs_[2],
                          regs_[8], regs_[9], regs_[10], regs_[11]});

  _calleeSaves = new temp::TempList(
      {regs_[1], regs_[6], regs_[12], regs_[13], regs_[14], regs_[15]});

  _returnSink = new temp::TempList({regs_[1], regs_[6], regs_[12], regs_[13],
                                    regs_[14], regs_[15], regs_[0], regs_[7]});
}

X64RegManager::~X64RegManager() {
  delete _registers;
  delete _argRegs;
  delete _callerSaves;
  delete _calleeSaves;
  delete _returnSink;
}

tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *stm) {
  tree::Stm *result = stm;
  std::list<tree::Stm *> viewShiftList = frame->viewShift->GetList();
  if (viewShiftList.size() == 0) {
    return result;
  }
  for (auto it = viewShiftList.begin(); it != viewShiftList.end(); it++) {
    result = new tree::SeqStm((*it), result);
  }
  return result;
}

void ProcEntryExit2(assem::InstrList *body) { 
  body->Append(
      new assem::OperInstr("", reg_manager->ReturnSink(), nullptr, nullptr));
}

assem::Proc *ProcEntryExit3(frame::Frame *frame, assem::InstrList *body) {
  static char instr[256] = {};
  std::string prolog;
  sprintf(instr, "%s:\n", frame->name_->Name().c_str());
  prolog = std::string(instr);

  sprintf(instr, "\tsubq $%d, %%rsp\n", frame->frame_size_);
  prolog.append(std::string(instr));

  sprintf(instr, "\taddq $%d, %%rsp\n", frame->frame_size_);
  std::string epilog = std::string(instr);

  epilog.append(std::string("\tretq\n"));
  return new assem::Proc(prolog, body, epilog);
}

} // namespace frame