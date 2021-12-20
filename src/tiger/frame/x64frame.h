//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  temp::Label *GetLabel() { return name_; }
  virtual Access *allocLocal(bool escape);
  static Frame *newFrame(temp::Label *name, std::vector<bool> *boolList);
  static tree::Exp *exp(frame::Access *access, tree::Exp *framePtr);
  static tree::Exp *externalCall(const std::string &s, tree::ExpList *args);
  static const int K = 14;
};

class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
public:
  X64RegManager();

  ~X64RegManager();

  temp::TempList *Registers() { return _registers; }

  temp::TempList *ArgRegs() { return _argRegs; }

  temp::TempList *CallerSaves() { return _callerSaves; }

  temp::TempList *CalleeSaves() { return _calleeSaves; }

  temp::TempList *ReturnSink() { return _returnSink; }

  int WordSize() { return 8; }
  int RegNum() { return 14; }

  temp::Temp *FramePointer() { return regs_[6]; }

  temp::Temp *StackPointer() { return regs_[7]; }

  temp::Temp *ReturnValue() { return regs_[0]; }

private:
  temp::TempList *_registers;
  temp::TempList *_argRegs;
  temp::TempList *_callerSaves;
  temp::TempList *_calleeSaves;
  temp::TempList *_returnSink;
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
