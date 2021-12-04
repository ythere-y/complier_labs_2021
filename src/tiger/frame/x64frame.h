//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
private:
  temp::TempList fast_get(int start, int end) {
    temp::TempList *res;
    for (int i = start; i < end; i++) {
      res->Append(regs_[i]);
    }
    return res;
  }
  temp::TempList fast_get(int start_1, int end_1, int start_2, int end_2) {
    temp::TempList *res;
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
    for (int i = 0; i < 32; i++) {
      regs_.push_back(temp::TempFactory::NewTemp());
    }
    { // map增加元素
      temp_map_->Enter(regs_[0], &std::string("$zero"));
      temp_map_->Enter(regs_[1], &std::string("$at"));
      temp_map_->Enter(regs_[2], &std::string("$v0"));
      temp_map_->Enter(regs_[3], &std::string("$v1"));
      temp_map_->Enter(regs_[4], &std::string("$a0"));
      temp_map_->Enter(regs_[5], &std::string("$a1"));
      temp_map_->Enter(regs_[6], &std::string("$a2"));
      temp_map_->Enter(regs_[7], &std::string("$a3"));
      temp_map_->Enter(regs_[8], &std::string("$t0"));
      temp_map_->Enter(regs_[9], &std::string("$t1"));
      temp_map_->Enter(regs_[10], &std::string("$t2"));
      temp_map_->Enter(regs_[11], &std::string("$t3"));
      temp_map_->Enter(regs_[12], &std::string("$t4"));
      temp_map_->Enter(regs_[13], &std::string("$t5"));
      temp_map_->Enter(regs_[14], &std::string("$t6"));
      temp_map_->Enter(regs_[15], &std::string("$t7"));
      temp_map_->Enter(regs_[16], &std::string("$s0"));
      temp_map_->Enter(regs_[17], &std::string("$s1"));
      temp_map_->Enter(regs_[18], &std::string("$s2"));
      temp_map_->Enter(regs_[19], &std::string("$s3"));
      temp_map_->Enter(regs_[20], &std::string("$s4"));
      temp_map_->Enter(regs_[21], &std::string("$s5"));
      temp_map_->Enter(regs_[22], &std::string("$s6"));
      temp_map_->Enter(regs_[23], &std::string("$s7"));
      temp_map_->Enter(regs_[24], &std::string("$t8"));
      temp_map_->Enter(regs_[25], &std::string("$t9"));
      temp_map_->Enter(regs_[26], &std::string("$k0"));
      temp_map_->Enter(regs_[27], &std::string("$k1"));
      temp_map_->Enter(regs_[28], &std::string("$gp"));
      temp_map_->Enter(regs_[29], &std::string("$sp"));
      temp_map_->Enter(regs_[30], &std::string("$fp"));
      temp_map_->Enter(regs_[31], &std::string("$ra"));
    }
  }
  ~X64RegManager() {}

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  temp::TempList *Registers() { return fast_get(0, 32); }

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  temp::TempList *ArgRegs() { return fast_get(4, 8); }

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  // caller save
  //寄存器。当过程P调用Q时，Q可以覆盖这些寄存器，而不会破坏P所需要的数据。
  temp::TempList *CallerSaves() { return fast_get(8, 16, 24, 26); }

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  // callee save
  // 寄存器。这意味着Q必须在覆盖它们之前，将这些寄存器的值保存到栈中，并在返回前恢复它们，因为P（或某个更高层次的过程）可能会在今后的计算中需要这些值。此外，根据这里描述的惯例，必须保存寄存器ebp，esp。
  temp::TempList *CalleeSaves() { return fast_get(16, 24); }

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  temp::TempList *ReturnSink() { return fast_get(2, 4); }

  /**
   * Get word size
   */
  int WordSize() { return 8; }
  temp::Temp *FramePointer() { return regs_[30]; }
  temp::Temp *StackPointer() { return regs_[29]; }
  temp::Temp *ReturnValue() { return regs_[31]; }
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
