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
    return new tree::MemExp(new tree::ConstExp(-offset), framePtr);
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
  this->s_offset_ = -word_size;
  this->frame_num_ = 0;
  this->frame_size_ = 0;
  this->args_size_ = 0;

  if (!escapes) //是main函数啦
  {
    return;
  }
  int count = 0; //总的参数的计数器
  int one_size = reg_manager->WordSize();
  int tmp_offset = 0;
  int to_frame_num = 0;                      // 要放到frame中的编号
  int to_reg_num = 0;                        // 要放到reg中的编号
  int total_num = escapes->size();           // 得到所有参数数量
  int arg_size = (total_num - 5) * one_size; // 传递参数所用的空间
  if (total_num <= 6)
    arg_size = 0; // 如果参数数量不超过6，那么就不需要栈传递
  this->args_size_ = arg_size;
  // 先记录frame的size

  for (auto it_es = escapes->begin(); it_es != escapes->end(); it_es++) {
    if (*it_es)
      frame_size_ += 8;
  }
  if (frame_size_ < 8)
    frame_size_ = 8;

  FLOG("escape [size = %d]\n", escapes->size());
  //最后，需要得到返回地址
  /*
  if (total_num > 6) {
    int ret_offset = (5 - total_num) * 8;
    temp::Temp *reg = temp::TempFactory::NewTemp();
    //这里的顺序是逆序的，不知道为什么
    view_shift_->Append(new tree::MoveStm(
        new tree::TempExp(reg_manager->StackPointer()),
        new tree::BinopExp(
            tree::BinOp::PLUS_OP, new tree::ConstExp(-8),
            new tree::TempExp(reg_manager->StackPointer())))); //将rsp移动回去
    //  将ret放到当前rsp中
    view_shift_->Append(new tree::MoveStm(
        new tree::MemExp(new tree::ConstExp(-8),
                         new tree::TempExp(reg_manager->StackPointer())),
        new tree::TempExp(reg)));
    // 将stack中的ret地址取出
    view_shift_->Append(new tree::MoveStm(
        new tree::TempExp(reg),
        new tree::MemExp(new tree::ConstExp(ret_offset),
                         new tree::TempExp(reg_manager->StackPointer()))));
    // rsp-8
  }
  */

  // 得到参数，分配到frame或者reg中
  for (auto it_es = escapes->begin(); it_es != escapes->end(); it_es++) {
    Access *add_ac;
    if (*it_es) {
      FLOG("one escaped[size = %d]\n", frame_size_);
      // 需要存frame中
      if (count < 6) {
        // 确定在frame中的位置--相对于rsp的偏移量
        tmp_offset = frame_size_ - 8 - to_frame_num * one_size;
        add_ac = new InFrameAccess(to_frame_num * one_size);
        // 增加一手将这个位置的东西移过来的操作
        view_shift_->Append(new tree::MoveStm(
            new tree::MemExp(new tree::ConstExp(tmp_offset),
                             new tree::TempExp(reg_manager->StackPointer())),
            new tree::TempExp(reg_manager->ArgRegs()->NthTemp(count))));
        to_frame_num++;
      } else {
        // 要从stack中取值
        // 这里计算相对于rsp的位移
        tmp_offset = frame_size_ - 8 - to_frame_num * one_size;
        //获取这个参数在stack中的offset;是负数
        int stack_offset = -(total_num - count) * one_size;
        // 这里记录的是相对于framepointer的位移
        add_ac = new InFrameAccess(to_frame_num * one_size);
        temp::Temp *reg = temp::TempFactory::NewTemp();
        // 将stack中的内容移动到临时的寄存器中
        view_shift_->Append(new tree::MoveStm(
            new tree::TempExp(reg),
            new tree::MemExp(new tree::ConstExp(stack_offset),
                             new tree::TempExp(reg_manager->StackPointer()))));
        //  将临时寄存器的值放到frame中
        view_shift_->Append(new tree::MoveStm(
            new tree::MemExp(new tree::ConstExp(tmp_offset),
                             new tree::TempExp(reg_manager->StackPointer())),
            new tree::TempExp(reg)));
        to_frame_num++;
      }
    } else {
      // 需要放在寄存器中
      temp::Temp *reg = temp::TempFactory::NewTemp(); // 存放最终结果的寄存器
      if (count < 6) {
        // 可以从寄存器中取值
        // 增加一手从args的地方拿来参数的操作
        view_shift_->Append(new tree::MoveStm(
            new tree::TempExp(reg),
            new tree::TempExp(reg_manager->ArgRegs()->NthTemp(count))));

      } else {
        // 要从stack中取值
        //获取这个参数在stack中的offset;是负数
        int stack_offset = -(total_num - count) * one_size;
        view_shift_->Append(new tree::MoveStm(
            new tree::TempExp(reg),
            new tree::MemExp(new tree::ConstExp(stack_offset),
                             new tree::TempExp(reg_manager->StackPointer()))));
      }
      add_ac = new InRegAccess(reg);
    }

    fromals_->push_back(add_ac);
    count++;
  }
  /*
  if (total_num > 6) {
    int stack_recover = (total_num - 6) * 8; // 每多一个，rsp就要移动8

    view_shift_->Append(new tree::MoveStm(
        new tree::TempExp(reg_manager->StackPointer()),
        new tree::BinopExp(
            tree::BinOp::PLUS_OP, new tree::ConstExp(stack_recover + 8),
            new tree::TempExp(reg_manager->StackPointer())))); //将rsp移动回去
  }
  */
}
Access *X64Frame::allocLocal(bool escape) {

  Access *local;
  if (escape) {
    local = new InFrameAccess(frame_size_);
    frame_size_ += 8;
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
          frame->frame_size_);
  prolog = std::string(instr);
  sprintf(instr, "%s:\n\t subq $%d,%%rsp\n", frame->label_->Name().c_str(),
          frame->frame_size_);
  // sprintf(instr, "\tsubq $%s_framesize, %%rsp\n",
  //         frame->label_->Name().c_str());
  prolog.append(std::string(instr));

  // sprintf(instr, "\t addq %d,%%rsp\n", frame->frame_size_);
  sprintf(instr, "\taddq $%d, %%rsp\n", frame->frame_size_,
          frame->label_->Name().c_str());
  // sprintf(instr, "\t \n");
  std::string epilog = std::string(instr);
  epilog.append(std::string("\tretq\n\n"));
  FLOG("exit 3 finished ~~~\n");
  return new assem::Proc(prolog, instr_list, epilog);
  /*H;
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