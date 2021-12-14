#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <memory>
#include <string>

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"

namespace frame {
#define FLOG(format, args...)                                                  \
  do {                                                                         \
    FILE *debug_log = fopen("tiger.log", "a+");                                \
    fprintf(debug_log, "%d,%s: ", __LINE__, __func__);                         \
    fprintf(debug_log, format, ##args);                                        \
    fclose(debug_log);                                                         \
  } while (0)

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;
  /**
   * Get rdx registers
   * @return rdx registers
   */
  [[nodiscard]] virtual temp::Temp *RDX() = 0;

  /**
   * Get word size
   */
  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  temp::Map *temp_map_;

protected:
  std::vector<temp::Temp *> regs_;
};

class Access {
public:
  /* TODO: Put your lab5 code here */
  Access() {}
  virtual ~Access() = default;
  virtual tree::Exp *ToExp(tree::Exp *framePtr) const = 0;
};

class Frame {
  /* TODO: Put your lab5 code here */
public:
  temp::Label *label_;
  std::vector<Access *> *fromals_;
  std::vector<Access *> *locals_;
  tree::StmList *view_shift_;
  int frame_num_;
  int frame_size_; // frame的大小
  int args_size_;  // args占据的大小
  int s_offset_;
  int maxArgs = 0;

  Frame() {}
  Frame(temp::Label *name, std::vector<bool> *escapes);

  virtual temp::Label *GetLabel() = 0;
  virtual temp::Label *get_name() = 0;
  virtual std::vector<Access *> *get_formals() = 0;
  virtual Access *allocLocal(bool escape) = 0;
};

/**
 * Fragments
 */

tree::Exp *externalCall(std::string s, tree::ExpList *args);
tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *stm);
assem::InstrList *ProcEntryExit2(assem::InstrList *instr_list);
assem::Proc *ProcEntryExit3(frame::Frame *frame, assem::InstrList *instr_list);

class Frag {
public:
  virtual ~Frag() = default;

  enum OutputPhase {
    Proc,
    String,
  };

  /**
   *Generate assembly for main program
   * @param out FILE object for output assembly file
   */
  virtual void OutputAssem(FILE *out, OutputPhase phase,
                           bool need_ra) const = 0;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) {
    body_ = frame::ProcEntryExit1(frame, body);
    frame_ = frame;
  }

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.emplace_back(frag); }
  const std::list<Frag *> &GetList() { return frags_; }

private:
  std::list<Frag *> frags_;
};

/* TODO: Put your lab5 code here */

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  X64Frame(){};
  temp::Label *GetLabel() { return label_; }
  X64Frame(temp::Label *name, std::vector<bool> *escapes);
  Access *allocLocal(bool escape);
  temp::Label *get_name() { return this->label_; }
  std::vector<Access *> *get_formals() { return this->fromals_; }
};
} // namespace frame

#endif