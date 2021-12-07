#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;

} // namespace

namespace cg {
static temp::TempList *saved;
static void saveCalleeRegs(assem::InstrList &instr_list, std::string_view fs);
static void restoreCalleeRegs(assem::InstrList &instr_list,
                              std::string_view fs);

void CodeGen::Codegen() { /* TODO: Put your lab5 code here */
  for (int i = 0; i < 6; i++) {
    saved->Append(temp::TempFactory::NewTemp());
  }
  assem::InstrList instr_list;

  saveCalleeRegs(instr_list, fs_);
  auto get_stm = traces_.get()->GetStmList()->GetList();
  auto it_stm = get_stm.begin();
  for (; it_stm != get_stm.end(); it_stm++) {
    (*it_stm)->Munch(instr_list, fs_);
  }
  restoreCalleeRegs(instr_list, fs_);
  // TODO:需要rsp和RV，是啥？
  temp::TempList *retlist = reg_manager->ReturnSink();

  assem::Targets *jumps = new assem::Targets(nullptr);
  instr_list.Append(new assem::OperInstr("", nullptr, retlist, jumps));
}

static void saveCalleeRegs(assem::InstrList &instr_list, std::string_view fs) {
  instr_list.Append(
      new assem::MoveInstr("movq `s0,`d0", saved, reg_manager->CalleeSaves()));
}

static void restoreCalleeRegs(assem::InstrList &instr_list,
                              std::string_view fs) {
  instr_list.Append(
      new assem::MoveInstr("movq `s0,`d0", reg_manager->CalleeSaves(), saved));
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // tree::MoveStm *s = (tree::MoveStm*)
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
  return;
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  assem::LabelInstr *res = new assem::LabelInstr(label_->Name(), label_);
  instr_list.Append(res);
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Label *label = exp_->name_;
  temp::TempList *dst = nullptr;
  temp::TempList *src = nullptr;
  assem::Targets *jumps = new assem::Targets(jumps_);

  std::string assem_name =
      std::string("jmp ").append(temp::LabelFactory::LabelString(label));

  instr_list.Append(new assem::OperInstr(assem_name, dst, src, jumps));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::TempList *dst = nullptr;
  temp::TempList *src = nullptr;
  assem::Targets *jumps = new assem::Targets(nullptr);

  temp::Temp *left = left_->Munch(instr_list, fs);
  temp::Temp *right = right_->Munch(instr_list, fs);

  src = new temp::TempList(left);
  src->Append(right);
  instr_list.Append(new assem::OperInstr("cmp `s0,`s1", nullptr, src, jumps));

  std::string str;
  switch (op_) {
  case tree::RelOp::EQ_OP:
    str = std::string("je ");
    break;
  case tree::RelOp::NE_OP:
    str = std::string("jne ");
    break;
  case tree::RelOp::LT_OP:
    str = std::string("jl ");
    break;
  case tree::RelOp::GT_OP:
    str = std::string("jg ");
    break;
  case tree::RelOp::LE_OP:
    str = std::string("jle ");
    break;
  case tree::RelOp::GE_OP:
    str = std::string("jge ");
    break;
  }
  std::string assem_name =
      str.append(temp::LabelFactory::LabelString(true_label_));
  std::vector<temp::Label *> *tmp_labels =
      new std::vector<temp::Label *>(1, true_label_);
  jumps = new assem::Targets(tmp_labels);
  instr_list.Append(new assem::OperInstr(assem_name, nullptr, nullptr, jumps));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::TempList *dst = nullptr;
  temp::TempList *src = nullptr;
  assem::Targets *jumps = new assem::Targets(nullptr);

  if (typeid(dst_) == typeid(tree::TempExp)) {
    dst = new temp::TempList(dst_->Munch(instr_list, fs));
    src = new temp::TempList(src_->Munch(instr_list, fs));
    instr_list.Append(new assem::MoveInstr("movq `s0,`d0", dst, src));
  } else if (typeid(dst_) == typeid(tree::MemExp)) {
    src = new temp::TempList(dst_->Munch(instr_list, fs));
    src->Append(src_->Munch(instr_list, fs));

    instr_list.Append(
        new assem::OperInstr("movq `s0,(`s1)", nullptr, src, jumps));
  } else {
    printf("Wrong tree::MoveStm.");
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *left = left_->Munch(instr_list, fs);
  temp::Temp *right = right_->Munch(instr_list, fs);
  instr_list.Append(new assem::MoveInstr(
      "movq `s0,d0", new temp::TempList(reg_manager->ReturnValue()),
      new temp::TempList(left)));
  temp::Temp *reg = temp::TempFactory::NewTemp();
  temp::TempList *dst;
  temp::TempList *src;
  assem::Targets *jumps = new assem::Targets(nullptr);
  dst = new temp::TempList(reg);
  src = new temp::TempList(right);
  src->Append(reg);

  switch (op_) {
  case tree::PLUS_OP:
    instr_list.Append(new assem::OperInstr("addq `s0,`d0", dst, src, jumps));
    break;
  case tree::MINUS_OP:
    instr_list.Append(new assem::OperInstr("subq `s0,`d0", dst, src, jumps));
  case tree::MUL_OP:
    instr_list.Append(new assem::OperInstr("imulq `s0,`d0", dst, src, jumps));
    break;
  case tree::DIV_OP:

    instr_list.Append(new assem::OperInstr(
        "cltd", reg_manager->ReturnSink(),
        new temp::TempList(reg_manager->ReturnValue()), jumps));

    auto get_return_sink = reg_manager->ReturnSink()->GetList();
    for (auto it_return_sink = get_return_sink.begin();
         it_return_sink != get_return_sink.end(); it_return_sink++)
      src->Append((*it_return_sink));

    dst = reg_manager->ReturnSink();
    instr_list.Append(new assem::OperInstr("idivq `s0", dst, src, jumps));
    src = new temp::TempList(reg_manager->ReturnValue());
    instr_list.Append(new assem::MoveInstr("movq `s0,`d0", dst, src));
    break;
  }
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *r = exp_->Munch(instr_list, fs);
  temp::Temp *reg = temp::TempFactory::NewTemp();
  instr_list.Append(
      new assem::OperInstr("movq (`s0),`d0", new temp::TempList(reg),
                           new temp::TempList(r), new assem::Targets(nullptr)));
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  return temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  char temp[256];
  sprintf(temp, "\tleaq %s(%%rip), `d0 ", (name_->Name().c_str()));
  temp::Temp *reg = temp::TempFactory::NewTemp();
  instr_list.Append(new assem::MoveInstr(std::string(temp),
                                         new temp::TempList(reg), nullptr));
  return reg;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  char temp[256];
  sprintf(temp, "movq $s%d, `d0 ", consti_);
  temp::Temp *reg = temp::TempFactory::NewTemp();
  temp::TempList *dst = new temp::TempList(reg);
  assem::Targets *jumps = new assem::Targets(nullptr);
  instr_list.Append(
      new assem::OperInstr(std::string(temp), dst, nullptr, jumps));
  return reg;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */

  temp::TempList *args_list = args_->MunchArgs(instr_list, fs);
  char temp[256];
  sprintf(
      temp, "call %s",
      temp::LabelFactory::LabelString(((tree::NameExp *)fun_)->name_).c_str());

  temp::TempList *dst = reg_manager->CalleeSaves();
  temp::TempList *src = nullptr;
  assem::Targets *jumps = new assem::Targets(nullptr);

  instr_list.Append(
      new assem::OperInstr(std::string(temp), dst, nullptr, jumps));

  temp::Temp *add_one = temp::TempFactory::NewTemp();
  dst = new temp::TempList(add_one);
  src = new temp::TempList(reg_manager->ReturnValue());
  instr_list.Append(new assem::MoveInstr("movq `s0,`d0", dst, src));
  return add_one;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list,
                                   std::string_view fs) {
  /* TODO: Put your lab5 code here */

  temp::TempList *res = new temp::TempList();
  temp::TempList *dst = nullptr;
  temp::TempList *src = nullptr;
  assem::Targets *jumps = new assem::Targets(nullptr);
  auto get_list = GetList();
  auto it_list = get_list.begin();
  for (int num = 1; it_list != get_list.end(); num++, it_list++) {
    temp::Temp *arg = (*it_list)->Munch(instr_list, fs);
    src = new temp::TempList(arg);
    if (reg_manager->GetRegister(num)) {
      temp::Temp *nth_one = reg_manager->ArgRegs()->NthTemp(num);
      dst = new temp::TempList(nth_one);
      instr_list.Append(new assem::MoveInstr("movq `s0,`d0", dst, src));
      res->Append(nth_one);
    } else {
      instr_list.Append(new assem::OperInstr("pushq `s0", nullptr, src, jumps));
    }
  }
  return res;
}

} // namespace tree
