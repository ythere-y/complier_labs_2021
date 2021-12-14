#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;

} // namespace
#define TAN CLOG("get here\n")
#define CLOG(format, args...)                                                  \
  do {                                                                         \
    FILE *debug_log = fopen("tiger.log", "a+");                                \
    fprintf(debug_log, "%d,%s: ", __LINE__, __func__);                         \
    fprintf(debug_log, format, ##args);                                        \
    fclose(debug_log);                                                         \
  } while (0)

namespace cg {
#define SAME(type_a, type_b) typeid(*(type_a)) == typeid(type_b)
#define IS_PLUS(type) (type)->op_ == tree::BinOp::PLUS_OP
temp::TempList *saved;
static void saveCalleeRegs(assem::InstrList &instr_list, std::string_view fs);
static void restoreCalleeRegs(assem::InstrList &instr_list,
                              std::string_view fs);

void CodeGen::Codegen() { /* TODO: Put your lab5 code here */
  CLOG("arrvied\n");
  assem::InstrList *instr_list = (assem_instr_.get()->GetInstrList());
  saveCalleeRegs(*instr_list, fs_);

  auto get_stm = traces_.get()->GetStmList()->GetList();
  auto it_stm = get_stm.begin();
  for (; it_stm != get_stm.end(); it_stm++) {
    (*it_stm)->Munch(*instr_list, fs_);
  }
  restoreCalleeRegs(*instr_list, fs_);
  CLOG("code gen finished ~~~~~\n");

  frame::ProcEntryExit2(instr_list);
}

static void saveCalleeRegs(assem::InstrList &instr_list, std::string_view fs) {
  saved = new temp::TempList();
  auto regs = reg_manager->CalleeSaves()->GetList();
  int len = regs.size();
  for (int i = 0; i < len; i++) {
    saved->Append(temp::TempFactory::NewTemp());
  }
  auto it_reg = regs.begin();
  auto get_saved = saved->GetList();
  auto it_saved = get_saved.begin();
  for (; it_reg != regs.end(); it_reg++, it_saved++)
    instr_list.Append(new assem::MoveInstr("movq `s0,`d0",
                                           new temp::TempList((*it_saved)),
                                           new temp::TempList((*it_reg))));
}

static void restoreCalleeRegs(assem::InstrList &instr_list,
                              std::string_view fs) {
  auto regs = reg_manager->CalleeSaves()->GetList();
  auto get_saved = saved->GetList();
  auto it_reg = regs.begin();
  auto it_saved = get_saved.begin();
  for (; it_reg != regs.end(); it_reg++, it_saved++)
    instr_list.Append(new assem::MoveInstr("movq `s0,`d0",
                                           new temp::TempList((*it_reg)),
                                           new temp::TempList((*it_saved))));
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
temp::TempList *L(temp::Temp *one, temp::Temp *two) {
  temp::TempList *res = L(one);
  res->Append(two);
  return res;
}
void PUSH(int nth, temp::Temp *src, assem::InstrList &instr_list,
          std::string_view fs) {
  char assem_name[80];
  sprintf(assem_name, "movq `s0,%d(`d0)", nth * 8);
  instr_list.Append(new assem::MoveInstr(
      std::string(assem_name), L(reg_manager->StackPointer()), L(src)));
}
void POP(int nth, temp::Temp *dst, assem::InstrList &instr_list,
         std::string_view fs) {
  char assem_name[80];
  sprintf(assem_name, "movq %d(`s0),`d0", nth * 8);
  instr_list.Append(new assem::OperInstr(
      assem_name, L(dst), L(reg_manager->StackPointer()), nullptr));
}

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // tree::MoveStm *s = (tree::MoveStm*)
  CLOG("[seqstm]\n");
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
  return;
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  CLOG("[Label stm]\n");
  assem::LabelInstr *res = new assem::LabelInstr(label_->Name(), label_);
  instr_list.Append(res);
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  CLOG("[Jump stm]\n");
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
  CLOG("[Cjump stm]\n");
  temp::TempList *dst = nullptr;
  temp::TempList *src = nullptr;
  assem::Targets *jumps = new assem::Targets(nullptr);

  temp::Temp *left = left_->Munch(instr_list, fs);
  temp::Temp *right = right_->Munch(instr_list, fs);

  src = L(left);
  dst = L(right);
  instr_list.Append(new assem::OperInstr("cmp `d0,`s0", dst, src, jumps));

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
  case tree::RelOp::ULT_OP:
    str = std::string("jb ");
    break;
  case tree::RelOp::ULE_OP:
    str = std::string("jbe ");
    break;
  case tree::RelOp::UGT_OP:
    str = std::string("jae ");
    break;
  case tree::RelOp::UGE_OP:
    str = std::string("ja ");
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

  CLOG("[Move stm]\n");
  temp::TempList *dst = nullptr;
  temp::TempList *src = nullptr;
  assem::Targets *jumps = new assem::Targets(nullptr);

  if (SAME(dst_, tree::TempExp)) {
    dst = new temp::TempList(dst_->Munch(instr_list, fs));
    src = new temp::TempList(src_->Munch(instr_list, fs));
    instr_list.Append(new assem::MoveInstr("movq `s0,`d0", dst, src));
  } else if (SAME(dst_, tree::MemExp)) {

    dst = L(((tree::MemExp *)dst_)->exp_->Munch(instr_list, fs));
    src = L(src_->Munch(instr_list, fs));

    instr_list.Append(new assem::OperInstr("movq `s0,(`d0)", dst, src, jumps));
  } else {
    printf("Wrong tree::MoveStm.");
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  CLOG("[Exp stm]\n");
  exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  CLOG("[Binop Exp ]\n");
  temp::Temp *left = left_->Munch(instr_list, fs);
  temp::Temp *right = right_->Munch(instr_list, fs);
  temp::Temp *reg = temp::TempFactory::NewTemp();
  temp::TempList *dst;
  temp::TempList *src;
  assem::Targets *jumps = new assem::Targets(nullptr);

  src = L(reg);
  src->Append(right);
  switch (op_) {

  case tree::PLUS_OP:
  case tree::MINUS_OP:
  case tree::MUL_OP: {
    dst = new temp::TempList(reg);
    src = new temp::TempList(left);
    // 将左边的数字先移动到结果寄存器
    instr_list.Append(new assem::MoveInstr("movq `s0,`d0", dst, src));
    src = L(right);
    switch (op_) {
    case tree::PLUS_OP:
      instr_list.Append(new assem::OperInstr("addq `s0,`d0", dst, src, jumps));
      break;
    case tree::MINUS_OP:
      instr_list.Append(new assem::OperInstr("subq `s0,`d0", dst, src, jumps));
      break;
    case tree::MUL_OP:
      instr_list.Append(new assem::OperInstr("imulq `s0,`d0", dst, src, jumps));
      break;
    }
    break;
  }

  case tree::DIV_OP: {
    // 被除数放到rax中
    instr_list.Append(new assem::MoveInstr(
        "movq `s0,`d0", L(reg_manager->ReturnValue()), L(left)));
    PUSH(1, reg_manager->RDX(), instr_list, fs);
    //对rax进行拓展，放到rdx中
    instr_list.Append(new assem::OperInstr("cqto", nullptr, nullptr, nullptr));
    src = L(right);
    // 除数作为参数操作数放入
    instr_list.Append(new assem::OperInstr("idivq `s0", dst, src, jumps));
    POP(1, reg_manager->RDX(), instr_list, fs);
    src = L(reg_manager->ReturnValue());
    dst = L(reg);
    // 将结果从rax中移到reg
    instr_list.Append(new assem::MoveInstr("movq `s0,`d0", dst, src));
    break;
  }
  }
  return reg;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  CLOG("[Mem Exp ]\n");
  temp::Temp *r = exp_->Munch(instr_list, fs);
  temp::Temp *reg = temp::TempFactory::NewTemp();
  instr_list.Append(
      new assem::OperInstr("movq (`s0),`d0", L(reg), L(r), nullptr));
  return reg;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  CLOG("[Temp Exp ]\n");
  return temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  CLOG("[Eseq Exp ]\n");
  stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  CLOG("[Name Exp ]\n");
  char temp[256];
  sprintf(temp, "\tleaq %s(%%rip), `d0 ", (name_->Name().c_str()));
  temp::Temp *reg = temp::TempFactory::NewTemp();
  instr_list.Append(new assem::MoveInstr(std::string(temp), L(reg), nullptr));
  return reg;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  CLOG("[Const Exp ]\n");
  char temp[256];
  sprintf(temp, "movq $%d, `d0 ", consti_);
  temp::Temp *reg = temp::TempFactory::NewTemp();
  temp::TempList *dst = L(reg);
  instr_list.Append(
      new assem::OperInstr(std::string(temp), dst, nullptr, nullptr));
  return reg;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  CLOG("[Call Exp ]\n");
  temp::TempList *dst = reg_manager->CalleeSaves();
  temp::TempList *src = nullptr;
  assem::Targets *jumps = new assem::Targets(nullptr);
  // prepare the params

  int frame_size = 0;
  static char instr[256];
  // sprintf(instr, "\tsubq $%s_framesize, %%rsp\n",
  //         temp::LabelFactory::LabelString(((NameExp *)fun_)->name_).c_str());
  // instr_list.Append(new assem::OperInstr(instr, nullptr, nullptr, nullptr));
  temp::TempList *args_list = args_->MunchArgs(frame_size, instr_list, fs);
  // sprintf(instr, "\taddq $%s_framesize, %%rsp\n",
  //         temp::LabelFactory::LabelString(((NameExp *)fun_)->name_).c_str());
  // instr_list.Append(new assem::OperInstr(instr, nullptr, nullptr, nullptr));
  int total_args = args_->GetList().size();
  /*
  if (total_args > 6) {
    sprintf(assem_name, "subq $%d,`d0",
            (total_args - 6) * reg_manager->WordSize());
    instr_list.Append(new assem::OperInstr(
        assem_name, L(reg_manager->StackPointer()), nullptr, nullptr));
  }
  */
  // call function
  char assem_name[80];

  sprintf(assem_name, "call %s",
          temp::LabelFactory::LabelString(((NameExp *)fun_)->name_).c_str());

  dst = reg_manager->CallerSaves();
  src = reg_manager->ArgRegs();
  instr_list.Append(
      new assem::OperInstr(std::string(assem_name), dst, src, nullptr));
  /*
    //参数过多，需要放栈
    if (total_args > 6) {
      sprintf(assem_name, "addq $%d,`d0",
              (total_arg - 6) * reg_manager->WordSize());
      instr_list.Append(new assem::OperInstr(
          assem_name, L(reg_manager->StackPointer()), nullptr, nullptr));
    }
    */
  temp::Temp *add_one = temp::TempFactory::NewTemp();
  src = L(reg_manager->ReturnValue());
  dst = L(add_one);
  instr_list.Append(new assem::MoveInstr("movq `s0,`d0", dst, src));
  return add_one;

  /*
  temp::TempList *args_list = args_->MunchArgs(instr_list, fs);
  temp::Temp *r = fun_->Munch(instr_list, fs);
  args_list->Append(r);

  instr_list.Append(new assem::OperInstr(
      "call `s0\n", reg_manager->CalleeSaves(), args_list, nullptr));
  return r;
  // TODO:这里修改较多

  instr_list.Append(
      new assem::OperInstr(std::string("hello"), dst, nullptr, jumps));

  temp::Temp *add_one = temp::TempFactory::NewTemp();
  dst = new temp::TempList(add_one);
  src = new temp::TempList(reg_manager->ReturnValue());
  instr_list.Append(new assem::MoveInstr("movq `s0,`d0", dst, src));
  return add_one;
  */
}

temp::TempList *ExpList::MunchArgs(int frame_size, assem::InstrList &instr_list,
                                   std::string_view fs) {
  /* TODO: Put your lab5 code here */
  CLOG("[Munch Args ]\n");

  temp::TempList *res = new temp::TempList();
  temp::TempList *dst = nullptr;
  temp::TempList *src = nullptr;
  assem::Targets *jumps = new assem::Targets(nullptr);

  auto get_list = GetList();
  auto it_list = get_list.begin();
  if (it_list == get_list.end())
    return res;
  // 遍历所有参数，每个参数找到一个合适的位置放
  int total_num = get_list.size();
  for (int num = 0; it_list != get_list.end(); it_list++) {
    temp::Temp *arg = (*it_list)->Munch(instr_list, fs);
    src = new temp::TempList(arg);

    if (num < 6) {
      // 从参数寄存器中抽取
      assert(num < 6 && num >= 0);
      temp::Temp *nth_one = reg_manager->ArgRegs()->NthTemp(num);
      dst = new temp::TempList(nth_one);
      instr_list.Append(new assem::MoveInstr("movq `s0,`d0", dst, src));
      res->Append(nth_one);
      num++;
    } else {
      //这里的push操作是隔空push
      PUSH(total_num - num, arg, instr_list, fs);
    }
  }
  return res;
}

} // namespace tree
