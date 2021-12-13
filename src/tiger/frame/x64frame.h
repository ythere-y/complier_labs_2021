//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {

static std::string reg_names[] = {
    "%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%rbp", "%rsp",
    "%r8",  "%r9",  "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};
// "$zero",
//   "$at",  "$v0",  "$v1",  "$a0",  "$a1",  "$a2",  "$a3",  "$t0",  "$t1",
//   "$t2",  "$t3",  "$t4",  "$t5",  "$t6",  "$t7",  "$s0",  "$s1",  "$s2",
//   "$s3",  "$s4",  "$s5",  "$s6",  "$s7",  "$t8",  "$t9",  "$k0",  "$k1",
//   "$gp",  "$sp",  "$fp",  "$ra"};
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
private:
  temp::TempList *fast_get(int start, int end) {
    temp::TempList *res = new temp::TempList();
    for (int i = start; i < end; i++) {
      res->Append(regs_[i]);
    }
    return res;
  }
  temp::TempList *fast_get(int start_1, int end_1, int start_2, int end_2) {
    temp::TempList *res = new temp::TempList();
    for (int i = start_1; i < end_1; i++) {
      res->Append(regs_[i]);
    }
    for (int i = start_2; i < end_1; i++) {
      res->Append(regs_[i]);
    }
    return res;
  }

public:
  X64RegManager() : RegManager() {
    //加入32个寄存器
    std::string *name;

    for (int i = 0; i < 16; i++) {
      regs_.push_back(temp::TempFactory::NewTemp());
    }
    for (int i = 0; i < 16; i++) {
      name = &(reg_names[i]);
      temp_map_->Enter(regs_[i], name);
    }
  }
  ~X64RegManager() {}

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  temp::TempList *Registers() { return fast_get(0, 16); }

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  temp::TempList *ArgRegs() {
    /*
     * rcx, rdx, rsi, rdi, r8, r9
     * rdi, rsi, rdx, rcx, r8, r9
     */
    int tmp_list[] = {5, 4, 3, 2, 8, 9};
    temp::TempList *res = new temp::TempList();

    for (int i = 0; i < 6; i++) {
      res->Append(regs_[tmp_list[i]]);
    }
    return res;
  }

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  // caller save
  //寄存器。当过程P调用Q时，Q可以覆盖这些寄存器，而不会破坏P所需要的数据。
  temp::TempList *CallerSaves() {
    /*
     * rax, rdx, rcx, rsi, rdi, r8, r9, r10,r11
     */
    int tmp_list[] = {0, 2, 3, 4, 5, 8, 9, 10, 11};
    temp::TempList *res = new temp::TempList();

    for (int i = 0; i < 9; i++) {
      res->Append(regs_[tmp_list[i]]);
    }
    return res;
  }
  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  // callee save
  // 寄存器。这意味着Q必须在覆盖它们之前，将这些寄存器的值保存到栈中，并在返回前恢复它们，因为P（或某个更高层次的过程）可能会在今后的计算中需要这些值。此外，根据这里描述的惯例，必须保存寄存器ebp，esp。
  temp::TempList *CalleeSaves() {
    /*
     * rbx, rbp, r12, r13, r14, r15
     */
    int tmp_list[] = {1, 6, 12, 13, 14, 15};
    temp::TempList *res = new temp::TempList();

    for (int i = 0; i < 6; i++) {
      res->Append(regs_[tmp_list[i]]);
    }
    return res;
  }

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  temp::TempList *ReturnSink() {
    temp::TempList *res = CalleeSaves();
    res->Append(ReturnValue());
    res->Append(StackPointer());
  }
  temp::Temp *RDX() { return regs_[3]; }

  /**
   * Get word size
   */
  int WordSize() { return 8; }
  temp::Temp *FramePointer() { return regs_[6]; }
  temp::Temp *StackPointer() { return regs_[7]; }
  temp::Temp *ReturnValue() { return regs_[0]; }
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
