#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"

#include <cassert>
#include <sstream>
#define TAN CLOG("get here\n")
#define CLOG(format, args...)                                                  \
  do {                                                                         \
    FILE *debug_log = fopen("tiger.log", "a+");                                \
    fprintf(debug_log, "%d,%s: ", __LINE__, __func__);                         \
    fprintf(debug_log, format, ##args);                                        \
    fclose(debug_log);                                                         \
  } while (0)

extern frame::RegManager *reg_manager;

static frame::Frame *_frame;

namespace {

constexpr int maxlen = 1024;

} // namespace

namespace cg {
#define SAME(type_a, type_b) typeid(*(type_a)) == typeid(type_b)
#define IS_PLUS(type) (type)->op_ == tree::BinOp::PLUS_OP
std::vector<temp::Temp *> calleeSaved(6);

void saveCalleeRegs(assem::InstrList &instr_list) {
  std::list<temp::Temp *> regList = reg_manager->CalleeSaves()->GetList();
  int i = 0;
  for (auto reg_it = regList.begin(); reg_it != regList.end(); reg_it++, i++) {
    calleeSaved[i] = temp::TempFactory::NewTemp();
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList({calleeSaved[i]}),
                                           new temp::TempList({*reg_it})));
  }
}

void restoreCalleeRegs(assem::InstrList &instr_list) {
  std::list<temp::Temp *> regList = reg_manager->CalleeSaves()->GetList();
  int i = 0;
  for (auto reg_it = regList.begin(); reg_it != regList.end(); reg_it++, i++) {
    instr_list.Append(
        new assem::MoveInstr("movq `s0, `d0", new temp::TempList({*reg_it}),
                             new temp::TempList({calleeSaved[i]})));
  }
}

void CodeGen::Codegen() { /* TODO: Put your lab5 code here */
  _frame = this->frame_;
  assem::InstrList *instrList = this->assem_instr_->GetInstrList();
  saveCalleeRegs(*instrList);
  // restore the frame size into a global register %rbx
  // NOTE: don't use %rbx in function body later!
  std::stringstream stream;
  stream << "movq $" << _frame->frame_size_ << ", `d0";
  instrList->Append(new assem::MoveInstr(
      stream.str(), new temp::TempList({reg_manager->GetRegister(1)}),
      nullptr));
  std::list<tree::Stm *> treeStmList = this->traces_->GetStmList()->GetList();
  for (auto stm_it = treeStmList.begin(); stm_it != treeStmList.end();
       stm_it++) {
    (*stm_it)->Munch(*instrList, fs_);
  }
  restoreCalleeRegs(*instrList);
  frame::ProcEntryExit2(instrList);
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */
temp::TempList *L(temp::Temp *inner) { return new temp::TempList(inner); }

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->left_->Munch(instr_list, fs);
  this->right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(
      temp::LabelFactory::LabelString(this->label_), this->label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::OperInstr("jmp `j0", nullptr, nullptr,
                                         new assem::Targets(this->jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *e1temp = this->left_->Munch(instr_list, fs);
  temp::Temp *e2temp = this->right_->Munch(instr_list, fs);
  temp::Label *trues = this->true_label_;
  temp::Label *falses = this->false_label_;

  std::string op;
  if (this->op_ == tree::EQ_OP) {
    op = "je";
  } else if (this->op_ == tree::NE_OP) {
    op = "jne";
  } else if (this->op_ == tree::LT_OP) {
    op = "jl";
  } else if (this->op_ == tree::LE_OP) {
    op = "jle";
  } else if (this->op_ == tree::GE_OP) {
    op = "jge";
  } else if (this->op_ == tree::GT_OP) {
    op = "jg";
  } else if (this->op_ == tree::ULT_OP) {
    op = "jb";
  } else if (this->op_ == tree::ULE_OP) {
    op = "jbe";
  } else if (this->op_ == tree::UGE_OP) {
    op = "jae";
  } else if (this->op_ == tree::UGT_OP) {
    op = "ja";
  }

  instr_list.Append(new assem::OperInstr(
      "cmpq `s1, `s0", nullptr, new temp::TempList({e1temp, e2temp}), nullptr));
  instr_list.Append(new assem::OperInstr(
      op + " `j0", nullptr, nullptr,
      new assem::Targets(new std::vector<temp::Label *>({trues, falses}))));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  tree::Exp *dst = this->dst_, *src = this->src_;
  if (typeid(*dst) == typeid(tree::MemExp)) {
    tree::MemExp *memDst = (tree::MemExp *)dst;
    if (typeid(*memDst->exp_) == typeid(tree::BinopExp) &&
        ((tree::BinopExp *)memDst->exp_)->op_ == tree::PLUS_OP &&
        typeid(*((tree::BinopExp *)memDst->exp_)->right_) ==
            typeid(tree::ConstExp)) {
      tree::Exp *e1 = ((tree::BinopExp *)memDst->exp_)->left_, *e2 = src;
      /** MOVE(MEM(BINOP(PLUS, e1, CONST(i)), e2) */
      temp::Temp *e1temp = e1->Munch(instr_list, fs);
      temp::Temp *e2temp = e2->Munch(instr_list, fs);
      std::stringstream stream;
      stream << "movq `s0, "
             << ((tree::ConstExp *)((tree::BinopExp *)memDst->exp_)->right_)
                    ->consti_
             << "(`s1)";
      std::string assem = stream.str();
      instr_list.Append(new assem::OperInstr(
          assem, nullptr, new temp::TempList({e2temp, e1temp}), nullptr));
    } else if (typeid(*memDst->exp_) == typeid(tree::BinopExp) &&
               ((tree::BinopExp *)memDst->exp_)->op_ == tree::PLUS_OP &&
               typeid(*((tree::BinopExp *)memDst->exp_)->left_) ==
                   typeid(tree::ConstExp)) {
      tree::Exp *e1 = ((tree::BinopExp *)memDst->exp_)->right_, *e2 = src;
      /** MOVE(MEM(BINOP(PLUS, CONST(i), e1), e2) */
      temp::Temp *e1temp = e1->Munch(instr_list, fs);
      temp::Temp *e2temp = e2->Munch(instr_list, fs);
      std::stringstream stream;
      stream << "movq `s0, "
             << ((tree::ConstExp *)((tree::BinopExp *)memDst->exp_)->left_)
                    ->consti_
             << "(`s1)";
      std::string assem = stream.str();
      assert(e1temp != reg_manager->FramePointer());
      assert(e2temp != reg_manager->FramePointer());
      instr_list.Append(new assem::OperInstr(
          assem, nullptr, new temp::TempList({e2temp, e1temp}), nullptr));
    } else if (typeid(*src) == typeid(tree::MemExp)) {
      tree::Exp *e1 = memDst->exp_, *e2 = ((tree::MemExp *)src)->exp_;
      /** MOVE(MEM(e1), MEM(e2)) */
      temp::Temp *t = temp::TempFactory::NewTemp();
      temp::Temp *e1temp = e1->Munch(instr_list, fs);
      temp::Temp *e2temp = e2->Munch(instr_list, fs);
      assert(e1temp != reg_manager->FramePointer());
      assert(e2temp != reg_manager->FramePointer());
      instr_list.Append(
          new assem::OperInstr("movq (`s0), `d0", new temp::TempList({t}),
                               new temp::TempList({e2temp}), nullptr));
      instr_list.Append(new assem::OperInstr("movq `s0, (`s1)", nullptr,
                                             new temp::TempList({t, e1temp}),
                                             nullptr));
    } else if (typeid(*memDst) == typeid(tree::ConstExp)) {
      assert(false);
      tree::Exp *e2 = src;
      /** MOVE(MEM(CONST(i)), e2) */
      temp::Temp *e2temp = e2->Munch(instr_list, fs);
      instr_list.Append(
          new assem::OperInstr("movq (some const), `s0", nullptr, //Δ
                               new temp::TempList({e2temp}), nullptr));
    } else {
      tree::Exp *e1 = memDst->exp_, *e2 = src;
      /** MOVE(MEM(e1), e2) */
      temp::Temp *e1temp = e1->Munch(instr_list, fs);
      temp::Temp *e2temp = e2->Munch(instr_list, fs);
      instr_list.Append(
          new assem::OperInstr("movq `s0, (`s1)", nullptr,
                               new temp::TempList({e2temp, e1temp}), nullptr));
    }
  } else if (typeid(*dst) == typeid(tree::TempExp)) {
    tree::Exp *e2 = src;
    /** MOVE(temp(i), e2) */
    temp::Temp *e2temp = e2->Munch(instr_list, fs);
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList({((tree::TempExp *)dst)->temp_}),
        new temp::TempList({e2temp})));
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *r = temp::TempFactory::NewTemp();
  switch (this->op_) {
  case tree::PLUS_OP: {
    tree::Exp *e1 = this->left_;
    tree::Exp *e2 = this->right_;
    temp::Temp *e1temp = e1->Munch(instr_list, fs);
    temp::Temp *e2temp = e2->Munch(instr_list, fs);
    instr_list.Append(
        new assem::OperInstr("movq `s0, `d0", new temp::TempList({r}),
                             new temp::TempList({e1temp}), nullptr));
    instr_list.Append(
        new assem::OperInstr("addq `s0, `d0", new temp::TempList({r}),
                             new temp::TempList({e2temp, r}), nullptr));
    return r;
  }
  case tree::MINUS_OP: {
    tree::Exp *e1 = this->left_;
    tree::Exp *e2 = this->right_;
    temp::Temp *e1temp = e1->Munch(instr_list, fs);
    temp::Temp *e2temp = e2->Munch(instr_list, fs);
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList({r}),
                                           new temp::TempList({e1temp})));
    instr_list.Append(
        new assem::OperInstr("subq `s0, `d0", new temp::TempList({r}),
                             new temp::TempList({e2temp, r}), nullptr));
    return r;
  }
  case tree::MUL_OP: {
    tree::Exp *e1 = this->left_;
    tree::Exp *e2 = this->right_;
    temp::Temp *e1temp = e1->Munch(instr_list, fs);
    temp::Temp *e2temp = e2->Munch(instr_list, fs);

    // Δ rax and rdx may need to be saved
    // temp::Temp *raxSaved = temp::TempFactory::NewTemp();
    // temp::Temp *rdxSaved = temp::TempFactory::NewTemp();
    // instr_list.Append(
    //     new assem::MoveInstr("movq `s0, `d0", new temp::TempList({raxSaved}),
    //                          new
    //                          temp::TempList({reg_manager->ReturnValue()})));
    // instr_list.Append(new assem::MoveInstr(
    //     "movq `s0, `d0", new temp::TempList({rdxSaved}),
    //     new temp::TempList({reg_manager->GetRegister(3)})));
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList({reg_manager->ReturnValue()}),
        new temp::TempList({e1temp})));
    instr_list.Append(new assem::OperInstr(
        "imulq `s0", nullptr, new temp::TempList({e2temp, r}), nullptr));

    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList({r}),
        new temp::TempList({reg_manager->ReturnValue()}))); //Δ higher 64 bits?
    // instr_list.Append(new assem::MoveInstr(
    //     "movq `s0, `d0", new temp::TempList({reg_manager->ReturnValue()}),
    //     new temp::TempList({raxSaved})));
    // instr_list.Append(new assem::MoveInstr(
    //     "movq `s0, `d0", new temp::TempList({reg_manager->GetRegister(3)}),
    //     new temp::TempList({rdxSaved})));
    return r;
  }
  case tree::DIV_OP: {
    tree::Exp *e1 = this->left_;
    tree::Exp *e2 = this->right_;
    temp::Temp *e1temp = e1->Munch(instr_list, fs);
    temp::Temp *e2temp = e2->Munch(instr_list, fs);
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList({reg_manager->ReturnValue()}),
        new temp::TempList({e1temp})));
    instr_list.Append(new assem::OperInstr(
        "cqto",
        new temp::TempList(
            {reg_manager->GetRegister(3), reg_manager->ReturnValue()}),
        new temp::TempList({reg_manager->ReturnValue()}), nullptr));
    instr_list.Append(
        new assem::OperInstr("idivq `s0",
                             new temp::TempList({reg_manager->GetRegister(3),
                                                 reg_manager->ReturnValue()}),
                             new temp::TempList({e2temp}), nullptr));
    instr_list.Append(
        new assem::MoveInstr("movq `s0, `d0", new temp::TempList({r}),
                             new temp::TempList({reg_manager->ReturnValue()})));
    return r;
  }
  }
  return r;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *src = exp_->Munch(instr_list, fs);
  temp::Temp *reg = temp::TempFactory::NewTemp();
  instr_list.Append(
      new assem::OperInstr("movq (`s0),`d0", L(reg), L(src), nullptr));
  return reg;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  return this->temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->stm_->Munch(instr_list, fs);
  return this->exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *r = temp::TempFactory::NewTemp();
  std::stringstream stream;
  stream << "leaq " << temp::LabelFactory::LabelString(this->name_)
         << "(%rip), `d0";
  std::string assem = stream.str();
  instr_list.Append(
      new assem::OperInstr(assem, new temp::TempList({r}), nullptr, nullptr));
  return r;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *r = temp::TempFactory::NewTemp();
  char assem_name[maxlen];
  sprintf(assem_name, "movq $%d,`d0", consti_);

  instr_list.Append(new assem::OperInstr(
      std::string(assem_name), new temp::TempList({r}), nullptr, nullptr));
  return r;
}

temp::TempList *param_regs(tree::ExpList *list) {
  int len = list->GetList().size();
  temp::TempList *tempList = new temp::TempList();
  for (int i = 0; i < len; i++) {
    tempList->Append(reg_manager->ArgRegs()->NthTemp(i));
  }
  return tempList;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *r = temp::TempFactory::NewTemp();
  std::string label =
      temp::LabelFactory::LabelString(((tree::NameExp *)this->fun_)->name_);
  this->args_->MunchArgs(instr_list, fs);
  std::string assem = std::string("callq ") + std::string(label);
  instr_list.Append(new assem::OperInstr(assem, reg_manager->CallerSaves(),
                                         param_regs(this->args_), nullptr));
  instr_list.Append(
      new assem::MoveInstr("movq `s0, `d0", new temp::TempList({r}),
                           new temp::TempList({reg_manager->ReturnValue()})));
  if (this->args_->GetList().size() > 6) {
    std::stringstream stream;
    stream << "\taddq $"
           << (this->args_->GetList().size() - 6) * reg_manager->WordSize()
           << ", `d0";
    instr_list.Append(new assem::MoveInstr(
        stream.str(), new temp::TempList({reg_manager->StackPointer()}),
        nullptr));
  }
  return r;
}

// Δ why not void
temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list,
                                   std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::TempList *res = new temp::TempList();
  std::list<tree::Exp *> expList = this->GetList();
  int i = 0;
  int totalnum = expList.size();
  for (auto exp_it = expList.begin(); exp_it != expList.end(); exp_it++, i++) {
    temp::Temp *arg = (*exp_it)->Munch(instr_list, fs);
    if (i < 6) {
      res->Append(reg_manager->ArgRegs()->NthTemp(i));
      instr_list.Append(new assem::MoveInstr(
          "movq `s0, `d0",
          new temp::TempList({reg_manager->ArgRegs()->NthTemp(i)}),
          new temp::TempList({arg})));
    } else {
      std::stringstream stream;
      stream << "movq `s0, " << -(totalnum - i) * reg_manager->WordSize()
             << "(`d0)";
      instr_list.Append(new assem::MoveInstr(
          stream.str(), new temp::TempList({reg_manager->StackPointer()}),
          new temp::TempList({arg})));
    }
  }
  if (totalnum > 6) {
    std::stringstream stream;
    stream << "subq $" << (totalnum - 6) * reg_manager->WordSize() << ", `d0";
    instr_list.Append(new assem::MoveInstr(
        stream.str(), new temp::TempList({reg_manager->StackPointer()}),
        nullptr));
  }
  return res;
}

} // namespace tree
