#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/x64frame.h"

#define test
#define TEMP temp::LabelFactory
#define DIFF(type_a, type_b) (typeid(*(type_a)) != typeid(type_b))
#define SAME(type_a, type_b) (typeid(*(type_a)) == typeid(type_b))
#define TAN LOG("get here\n");
#define BENULL LOG("someting null\n");
#define NONULL LOG("all not null\n");

#define COMMANLOG(format, level, label, args...)                               \
  do {                                                                         \
    LOG((format),                                                              \
        temp::LabelFactory::LabelString((level)->frame_->label_).c_str(),      \
        temp::LabelFactory::LabelString((label)).c_str(), ##args);             \
  } while (0)

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

static type::TyList *make_formal_tylist(sym::Table<type::Ty> *tenv,
                                        absyn::FieldList *params,
                                        err::ErrorMsg *errormsg) {
  if (params == nullptr) {
    return nullptr;
  }

  std::list<absyn::Field *> fieldList = params->GetList();
  type::TyList *tyList = new type::TyList();
  for (auto it = fieldList.begin(); it != fieldList.end(); it++) {
    type::Ty *ty = tenv->Look((*it)->typ_);
    if (ty == nullptr) {
      errormsg->Error((*it)->pos_, "undefined type %s",
                      (*it)->typ_->Name().c_str());
    }
    tyList->Append(ty->ActualTy());
  }
  return tyList;
}

static type::FieldList *make_fieldlist(int pos, sym::Table<type::Ty> *tenv,
                                       absyn::FieldList *fields,
                                       err::ErrorMsg *errormsg) {
  if (fields == nullptr) {
    return nullptr;
  }
  std::list<absyn::Field *> fieldList = fields->GetList();
  type::FieldList *typeFieldList = new type::FieldList();
  for (auto it = fieldList.begin(); it != fieldList.end(); it++) {
    type::Ty *ty = tenv->Look((*it)->typ_);
    if (ty == nullptr) {
      ty = type::IntTy::Instance();
      errormsg->Error(pos, "undefined type %s", (*it)->typ_->Name().c_str());
    }
    typeFieldList->Append(new type::Field((*it)->name_, ty));
  }

  return typeFieldList;

} // namespace

namespace tr {

void do_patch(temp::Label **tfs, temp::Label *label) { tfs[0] = label; }

Access *Access::allocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  frame::Access *fr_access = level->frame_->allocLocal(escape);
  tr::Access *access = new tr::Access(level, fr_access);
  return access;
}

std::vector<tr::Access *> *Level::Formals() {
  std::list<frame::Access *> fr_accessList = frame_->GetFormals();
  std::vector<tr::Access *> *tr_accessList = new std::vector<Access *>();
  for (auto it = fr_accessList.begin(); it != fr_accessList.end(); it++) {
    tr_accessList->push_back(new tr::Access(this, (*it)));
  }
  return tr_accessList;
}

tr::Level *Level::NewLevel(tr::Level *parent, temp::Label *name,
                           std::vector<bool> *formals) {
  formals->insert(formals->begin(), true);
  frame::Frame *frame = frame::X64Frame::newFrame(name, formals);
  tr::Level *level = new tr::Level(frame, parent);
  return level;
}

class Cx {
public:
  temp::Label **trues_;
  temp::Label **falses_;
  tree::Stm *stm_;

  Cx(temp::Label **trues, temp::Label **falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() const = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() const = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) const = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();

    tree::CjumpStm *stm = new tree::CjumpStm(tree::RelOp::NE_OP, exp_,
                                             new tree::ConstExp(0), t, f);
    temp::Label **trues = &(stm->true_label_);
    temp::Label **falses = &(stm->false_label_);
    return Cx(trues, falses, stm);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    assert(false);
    return Cx(nullptr, nullptr, nullptr);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(temp::Label **trues, temp::Label **falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel(),
                *f = temp::LabelFactory::NewLabel();

    do_patch(cx_.trues_, t);
    do_patch(cx_.falses_, f);

    return new tree::EseqExp(
        new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
        new tree::EseqExp(
            cx_.stm_,
            new tree::EseqExp(
                new tree::LabelStm(f),
                new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),
                                                    new tree::ConstExp(0)),
                                  new tree::EseqExp(new tree::LabelStm(t),
                                                    new tree::TempExp(r))))));
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(UnEx());
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

tr::Exp *translateSimpleVar(tr::Access *access, tr::Level *level) {

  tree::Exp *frameExp = new tree::BinopExp(
      tree::BinOp::PLUS_OP, new tree::TempExp(reg_manager->StackPointer()),
      new tree::TempExp(reg_manager->GetRegister(1)));

  while (level != access->level_) {
    frameExp = new tree::MemExp(new tree::BinopExp(
        tree::MINUS_OP, frameExp, new tree::ConstExp(reg_manager->WordSize())));
    level = level->parent_;
  }
  frameExp = frame::X64Frame::exp(access->access_, frameExp);
  return new tr::ExExp(frameExp);
}

tr::Exp *translateFieldVar(tr::Exp *exp, int offset) {
  return new tr::ExExp(new tree::MemExp(new tree::BinopExp(
      tree::PLUS_OP, exp->UnEx(),
      new tree::ConstExp(offset * reg_manager->WordSize()))));
}

tr::Exp *translateSubscriptVar(tr::Exp *var, tr::Exp *subscript) {
  return new tr::ExExp(new tree::MemExp(new tree::BinopExp(
      tree::PLUS_OP, var->UnEx(),
      new tree::BinopExp(tree::MUL_OP, subscript->UnEx(),
                         new tree::ConstExp(reg_manager->WordSize())))));
}

tr::Exp *translateStringExp(const std::string &str) {
  temp::Label *label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(label, str));
  return new tr::ExExp(new tree::NameExp(label));
}

tr::Exp *translateCall(temp::Label *label, std::list<tr::Exp *> expList,
                       tr::Level *callerLevel, tr::Level *calleeLevel) {
  tree::Exp *staticLink = new tree::BinopExp(
      tree::BinOp::PLUS_OP, new tree::TempExp(reg_manager->StackPointer()),
      new tree::TempExp(reg_manager->GetRegister(1)));
  tr::Level *levelp = callerLevel;
  while (levelp != calleeLevel->parent_) {
    // delete staticLink; // Δ seems the garbage will be delete somewhere else
    staticLink = new tree::MemExp(
        new tree::BinopExp(tree::MINUS_OP, staticLink,
                           new tree::ConstExp(reg_manager->WordSize())));
    levelp = levelp->parent_;
  }

  tree::ExpList *list = new tree::ExpList();
  for (const auto &exp : expList) {
    list->Append(exp->UnEx());
  }

  // transfer the static link and arguments
  if (calleeLevel->parent_) {
    // if the to-be-called function's level is on or below tigermain
    tree::ExpList *newList = new tree::ExpList();
    newList->Append(staticLink);
    std::list<tree::Exp *> treeExpList = list->GetList();
    for (auto it = treeExpList.begin(); it != treeExpList.end(); it++) {
      newList->Append(*it);
    }
    return new tr::ExExp(new tree::CallExp(new tree::NameExp(label), newList));
  } else { // if the function being called is external function
    return new tr::ExExp(frame::X64Frame::externalCall(
        temp::LabelFactory::LabelString(label), list));
  }
}

tr::Exp *calc(absyn::Oper op, tr::Exp *lhs, tr::Exp *rhs) {
  if (op == absyn::PLUS_OP) {
    return new tr::ExExp(
        new tree::BinopExp(tree::PLUS_OP, lhs->UnEx(), rhs->UnEx()));
  } else if (op == absyn::MINUS_OP) {
    return new tr::ExExp(
        new tree::BinopExp(tree::MINUS_OP, lhs->UnEx(), rhs->UnEx()));
  } else if (op == absyn::TIMES_OP) {
    return new tr::ExExp(
        new tree::BinopExp(tree::MUL_OP, lhs->UnEx(), rhs->UnEx()));
  } else if (op == absyn::DIVIDE_OP) {
    return new tr::ExExp(
        new tree::BinopExp(tree::DIV_OP, lhs->UnEx(), rhs->UnEx()));
  }
}

tr::Exp *stringEqual(tr::Exp *lhs, tr::Exp *rhs) {
  tree::ExpList *treeExpList = new tree::ExpList();
  treeExpList->Append(lhs->UnEx());
  treeExpList->Append(rhs->UnEx());
  return new tr::ExExp(
      frame::X64Frame::externalCall("string_equal", treeExpList));
}

tr::Exp *cmp(absyn::Oper op, tr::Exp *lhs, tr::Exp *rhs) {
  tree::CjumpStm *stm;
  if (op == absyn::EQ_OP) {
    stm = new tree::CjumpStm(tree::EQ_OP, lhs->UnEx(), rhs->UnEx(), nullptr,
                             nullptr);
  } else if (op == absyn::NEQ_OP) {
    stm = new tree::CjumpStm(tree::NE_OP, lhs->UnEx(), rhs->UnEx(), nullptr,
                             nullptr);
  } else if (op == absyn::LT_OP) {
    stm = new tree::CjumpStm(tree::LT_OP, lhs->UnEx(), rhs->UnEx(), nullptr,
                             nullptr);
  } else if (op == absyn::LE_OP) {
    stm = new tree::CjumpStm(tree::LE_OP, lhs->UnEx(), rhs->UnEx(), nullptr,
                             nullptr);
  } else if (op == absyn::GE_OP) {
    stm = new tree::CjumpStm(tree::GE_OP, lhs->UnEx(), rhs->UnEx(), nullptr,
                             nullptr);
  } else if (op == absyn::GT_OP) {
    stm = new tree::CjumpStm(tree::GT_OP, lhs->UnEx(), rhs->UnEx(), nullptr,
                             nullptr);
  }
  temp::Label **trues = &(stm->true_label_);
  temp::Label **falses = &(stm->false_label_);
  return new CxExp(trues, falses, stm);
}

tree::Stm *make_record(const std::vector<tr::Exp *> &expList, temp::Temp *r,
                       int offset) {
  if (expList.size() == 1) {
    return new tree::MoveStm(
        new tree::MemExp(new tree::BinopExp(
            tree::PLUS_OP, new tree::TempExp(r),
            new tree::ConstExp(offset * reg_manager->WordSize()))),
        expList[offset]->UnEx());
  } else if (offset == expList.size() - 2) {
    return new tree::SeqStm(
        new tree::MoveStm(
            new tree::MemExp(new tree::BinopExp(
                tree::PLUS_OP, new tree::TempExp(r),
                new tree::ConstExp(offset * reg_manager->WordSize()))),
            expList[offset]->UnEx()),
        new tree::MoveStm(
            new tree::MemExp(new tree::BinopExp(
                tree::PLUS_OP, new tree::TempExp(r),
                new tree::ConstExp((offset + 1) * reg_manager->WordSize()))),
            expList[offset + 1]->UnEx()));
  } else {
    return new tree::SeqStm(
        new tree::MoveStm(
            new tree::MemExp(new tree::BinopExp(
                tree::PLUS_OP, new tree::TempExp(r),
                new tree::ConstExp(offset * reg_manager->WordSize()))),
            expList[offset]->UnEx()),
        tr::make_record(expList, r, offset + 1));
  }
}

tr::Exp *translateRecordExp(std::vector<tr::Exp *> expList) {
  int count = expList.size();
  temp::Temp *r = temp::TempFactory::NewTemp();
  tree::ExpList *treeExpList = new tree::ExpList();
  treeExpList->Append(new tree::ConstExp(reg_manager->WordSize() * count));
  tree::Stm *stm = new tree::MoveStm(
      new tree::TempExp(r),
      frame::X64Frame::externalCall("alloc_record", treeExpList));

  stm = new tree::SeqStm(stm, tr::make_record(expList, r, 0));
  return new tr::ExExp(new tree::EseqExp(stm, new tree::TempExp(r)));
}

tr::Exp *translateSeq(tr::Exp *lhs, tr::Exp *rhs) {

  if (rhs) {
    return new tr::ExExp(new tree::EseqExp(lhs->UnNx(), rhs->UnEx()));
  } else {
    return new tr::ExExp(new tree::EseqExp(lhs->UnNx(), new tree::ConstExp(0)));
  }
}

tr::Exp *assign(tr::Exp *lhs, tr::Exp *rhs) {
  return new tr::NxExp(new tree::MoveStm(lhs->UnEx(), rhs->UnEx()));
}

tr::Exp *translateIf(tr::Exp *test_ty, tr::Exp *then, tr::Exp *elsee,
                     err::ErrorMsg *errormsg) {
  Cx cx = test_ty->UnCx(errormsg);
  temp::Temp *r = temp::TempFactory::NewTemp();
  temp::Label *t = temp::LabelFactory::NewLabel(),
              *f = temp::LabelFactory::NewLabel(),
              *final = temp::LabelFactory::NewLabel();
  do_patch(cx.trues_, t);
  do_patch(cx.falses_, f);
  if (elsee) {
    std::vector<temp::Label *> *labelList =
        new std::vector<temp::Label *>(1, final);
    return new ExExp(new tree::EseqExp(
        cx.stm_,
        new tree::EseqExp(
            new tree::LabelStm(t),
            new tree::EseqExp(
                new tree::MoveStm(new tree::TempExp(r), then->UnEx()),
                new tree::EseqExp(
                    new tree::JumpStm(new tree::NameExp(final), labelList),
                    new tree::EseqExp(
                        new tree::LabelStm(f),
                        new tree::EseqExp(
                            new tree::MoveStm(new tree::TempExp(r),
                                              elsee->UnEx()),
                            new tree::EseqExp(
                                new tree::JumpStm(new tree::NameExp(final),
                                                  labelList),
                                new tree::EseqExp(new tree::LabelStm(final),
                                                  new tree::TempExp(r))))))))));
  } else {
    return new NxExp(new tree::SeqStm(
        cx.stm_, new tree::SeqStm(
                     new tree::LabelStm(t),
                     new tree::SeqStm(then->UnNx(), new tree::LabelStm(f)))));
  }
}

tr::Exp *translateWhile(tr::Exp *test_ty, tr::Exp *body, temp::Label *doneLabel,
                        err::ErrorMsg *errormsg) {
  Cx testCx = test_ty->UnCx(errormsg);
  temp::Label *bodyLabel = temp::LabelFactory::NewLabel(),
              *testLabel = temp::LabelFactory::NewLabel();

  do_patch(testCx.trues_, bodyLabel);
  do_patch(testCx.falses_, doneLabel);

  std::vector<temp::Label *> *labelList =
      new std::vector<temp::Label *>(1, testLabel);
  return new tr::NxExp(new tree::SeqStm(
      new tree::LabelStm(testLabel),
      new tree::SeqStm(
          testCx.stm_,
          new tree::SeqStm(
              new tree::LabelStm(bodyLabel),
              new tree::SeqStm(
                  body->UnNx(),
                  new tree::SeqStm(new tree::JumpStm(
                                       new tree::NameExp(testLabel), labelList),
                                   new tree::LabelStm(doneLabel)))))));
}

tr::Exp *translateFor(frame::Access *access, tr::Level *level, tr::Exp *lo,
                      tr::Exp *hi, tr::Exp *body, temp::Label *doneLabel) {
  temp::Label *bodyLabel = temp::LabelFactory::NewLabel(),
              *testLabel = temp::LabelFactory::NewLabel();
  tree::Exp *i = frame::X64Frame::exp(
      access,
      new tree::BinopExp(tree::BinOp::PLUS_OP,
                         new tree::TempExp(reg_manager->StackPointer()),
                         new tree::TempExp(reg_manager->GetRegister(1))));
  std::vector<temp::Label *> *labelList =
      new std::vector<temp::Label *>(1, testLabel);
  return new tr::NxExp(new tree::SeqStm(
      new tree::MoveStm(i, lo->UnEx()),
      new tree::SeqStm(
          new tree::LabelStm(testLabel),
          new tree::SeqStm(
              new tree::CjumpStm(tree::LE_OP, i, hi->UnEx(), bodyLabel,
                                 doneLabel),
              new tree::SeqStm(
                  new tree::LabelStm(bodyLabel),
                  new tree::SeqStm(
                      body->UnNx(),
                      new tree::SeqStm(
                          new tree::MoveStm(
                              i, new tree::BinopExp(tree::PLUS_OP, i,
                                                    new tree::ConstExp(1))),
                          new tree::SeqStm(
                              new tree::JumpStm(new tree::NameExp(testLabel),
                                                labelList),
                              new tree::LabelStm(doneLabel)))))))));
}

tr::Exp *translateBreak(temp::Label *done) {
  std::vector<temp::Label *> *labelList =
      new std::vector<temp::Label *>(1, done);
  return new tr::NxExp(new tree::JumpStm(new tree::NameExp(done), labelList));
}

void translateFunctionDec(tr::Exp *body, tr::Level *level) {
  tree::Stm *stm = new tree::MoveStm(
      new tree::TempExp(reg_manager->ReturnValue()), body->UnEx());
  stm = frame::ProcEntryExit1(level->frame_, stm);
  frags->PushBack(new frame::ProcFrag(stm, level->frame_));
}

tr::Exp *translateArray(tr::Exp *size, tr::Exp *init) {
  tree::ExpList *argList = new tree::ExpList();
  argList->Append(size->UnEx());
  argList->Append(init->UnEx());
  return new tr::ExExp(frame::X64Frame::externalCall("init_array", argList));
}

void ProgTr::Translate() {

  /* TODO: Put your lab5 code here */
  FillBaseVEnv();
  FillBaseTEnv();

  frame::Frame *frame = frame::X64Frame::newFrame(
      temp::LabelFactory::NamedLabel("tigermain"), nullptr);
  Level *mainLevel = new Level(
      frame, main_level_.get()); // Δ is it a child of the outer level?
  temp::Label *mainLabel = temp::LabelFactory::NamedLabel("tigermain");

  tr::ExpAndTy *root_expty = absyn_tree_->Translate(
      venv_.get(), tenv_.get(), mainLevel, mainLabel, errormsg_.get());
  tr::translateFunctionDec(root_expty->exp_, mainLevel);
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return this->root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry *x = venv->Look(this->sym_);
  if (x && typeid(*x) == typeid(env::VarEntry)) {
    return new tr::ExpAndTy(
        tr::translateSimpleVar(((env::VarEntry *)x)->access_, level),
        ((env::VarEntry *)x)->ty_);
  } else {
    errormsg->Error(this->pos_, "undefined symbal");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var = this->var_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *varTy = var->ty_->ActualTy();

  if (typeid(*varTy) != typeid(type::RecordTy)) {
    errormsg->Error(this->pos_, "not a record type");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }

  std::list<type::Field *> list = ((type::RecordTy *)varTy)->fields_->GetList();
  int offset = 0;

  for (auto it = list.begin(); it != list.end(); it++) {
    type::Field *field = *it;
    if (field->name_ == this->sym_) {
      return new tr::ExpAndTy(tr::translateFieldVar(var->exp_, offset),
                              field->ty_);
    }
    offset++;
  }
  errormsg->Error(this->pos_, "field %s doesn'tree exist",
                  this->sym_->Name().c_str());
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var = this->var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *subscript =
      this->subscript_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *ty = var->ty_->ActualTy();
  if (typeid(*ty) != typeid(type::ArrayTy)) {
    errormsg->Error(this->pos_, "array type required");
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  return new tr::ExpAndTy(tr::translateSubscriptVar(var->exp_, subscript->exp_),
                          ((type::ArrayTy *)ty)->ty_->ActualTy());
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  if (typeid(*(this->var_)) == typeid(absyn::SimpleVar)) {
    if (!venv->Look(((SimpleVar *)this->var_)->sym_)) {
      errormsg->Error(this->pos_, "undefined variable %s",
                      ((SimpleVar *)this->var_)->sym_->Name().c_str());
      return new tr::ExpAndTy(nullptr, type::NilTy::Instance());
    }
    return ((absyn::SimpleVar *)this->var_)
        ->Translate(venv, tenv, level, label, errormsg);
  } else if (typeid(*(this->var_)) == typeid(absyn::SubscriptVar)) {
    return ((absyn::SubscriptVar *)this->var_)
        ->Translate(venv, tenv, level, label, errormsg);
  } else if (typeid(*(this->var_)) == typeid(absyn::FieldVar)) {
    return ((absyn::FieldVar *)this->var_)
        ->Translate(venv, tenv, level, label, errormsg);
  }
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                          type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(this->val_)),
                          type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(tr::translateStringExp(this->str_),
                          type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry *x = venv->Look(this->func_);

  std::list<type::Ty *> formals = ((env::FunEntry *)x)->formals_->GetList();
  std::list<Exp *> args = this->args_->GetList();
  type::Ty *result = ((env::FunEntry *)x)->result_;
  std::list<tr::Exp *> expList;
  auto formal_it = formals.begin();
  auto arg_it = args.begin();
  for (; formal_it != formals.end() && arg_it != args.end();
       formal_it++, arg_it++) {
    tr::ExpAndTy *arg_translated =
        (*arg_it)->Translate(venv, tenv, level, label, errormsg);
    type::Ty *argTy = arg_translated->ty_->ActualTy();
    expList.push_back(arg_translated->exp_);
  }
  return new tr::ExpAndTy(tr::translateCall(this->func_, expList, level,
                                            ((env::FunEntry *)x)->level_),
                          result);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *left =
      this->left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *right =
      this->right_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *leftTy = left->ty_->ActualTy();
  type::Ty *rightTy = right->ty_->ActualTy();

  if (this->oper_ == absyn::PLUS_OP || this->oper_ == absyn::MINUS_OP ||
      this->oper_ == absyn::TIMES_OP || this->oper_ == absyn::DIVIDE_OP) {
    return new tr::ExpAndTy(tr::calc(this->oper_, left->exp_, right->exp_),
                            type::IntTy::Instance());
  } else if (this->oper_ == absyn::EQ_OP &&
             typeid(*leftTy) == typeid(type::StringTy) &&
             typeid(*rightTy) == typeid(type::StringTy)) {
    return new tr::ExpAndTy(
        tr::stringEqual(left->exp_, right->exp_),
        type::IntTy::Instance()); //Δ can NE_OP be operator for string type?
  } else {
    return new tr::ExpAndTy(tr::cmp(this->oper_, left->exp_, right->exp_),
                            type::IntTy::Instance());
  }
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *ty = tenv->Look(this->typ_)->ActualTy();
  std::vector<tr::Exp *> expList;
  std::list<absyn::EField *> efieldList = this->fields_->GetList();
  for (auto it = efieldList.begin(); it != efieldList.end(); it++) {
    tr::ExpAndTy *expAndTy =
        (*it)->exp_->Translate(venv, tenv, level, label, errormsg);
    expList.push_back(expAndTy->exp_);
  }
  return new tr::ExpAndTy(tr::translateRecordExp(expList), ty);
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<Exp *> expList = this->seq_->GetList();
  tr::Exp *exp = new tr::ExExp(new tree::ConstExp(0));
  type::Ty *ty = nullptr;
  if (expList.size() == 0) {
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  for (auto it = expList.begin(); it != expList.end(); it++) {
    tr::ExpAndTy *expAndTy =
        (*it)->Translate(venv, tenv, level, label, errormsg);
    exp = tr::translateSeq(exp, expAndTy->exp_);
    ty = expAndTy->ty_;
  }
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var = this->var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *exp = this->exp_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *varTy = var->ty_->ActualTy();
  type::Ty *expTy = exp->ty_->ActualTy();

  if (typeid(*(this->var_)) == typeid(absyn::SimpleVar)) {
    env::EnvEntry *x = venv->Look(((SimpleVar *)this->var_)->sym_);
  }
  return new tr::ExpAndTy(tr::assign(var->exp_, exp->exp_),
                          type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *test_ty =
      this->test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *then =
      this->then_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *elsee = new tr::ExpAndTy(nullptr, nullptr);
  type::Ty *testTy = test_ty->ty_->ActualTy();
  type::Ty *thenTy = then->ty_->ActualTy();
  type::Ty *elseeTy = nullptr;
  if (this->elsee_) {
    elsee = this->elsee_->Translate(venv, tenv, level, label, errormsg);
    elseeTy = elsee->ty_->ActualTy();
  }

  return new tr::ExpAndTy(
      tr::translateIf(test_ty->exp_, then->exp_, elsee->exp_, errormsg),
      thenTy);
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  temp::Label *done = temp::LabelFactory::NewLabel();
  tr::ExpAndTy *test_ty =
      this->test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *body =
      this->body_->Translate(venv, tenv, level, done, errormsg);
  type::Ty *bodyTy = body->ty_;
  return new tr::ExpAndTy(
      tr::translateWhile(test_ty->exp_, body->exp_, done, errormsg), body->ty_);
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  venv->BeginScope();
  temp::Label *done = temp::LabelFactory::NewLabel();
  tr::ExpAndTy *lo = this->lo_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *hi = this->hi_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *loTy = lo->ty_;
  type::Ty *hiTy = hi->ty_;

  tr::Access *access = tr::Access::allocLocal(level, this->escape_);
  venv->Enter(this->var_, new env::VarEntry(access, loTy, true));

  tr::ExpAndTy *body =
      this->body_->Translate(venv, tenv, level, done, errormsg);
  type::Ty *bodyTy = body->ty_;

  venv->EndScope();
  return new tr::ExpAndTy(tr::translateFor(access->access_, level, lo->exp_,
                                           hi->exp_, body->exp_, done),
                          type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(tr::translateBreak(label), type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<Dec *> decList = this->decs_->GetList();
  tr::Exp *seq = nullptr;
  for (auto it = decList.begin(); it != decList.end(); it++) {
    if (seq) {
      seq = tr::translateSeq(
          seq, (*it)->Translate(venv, tenv, level, label, errormsg));
    } else {
      seq = (*it)->Translate(venv, tenv, level, label, errormsg);
    }
  }
  tr::ExpAndTy *body =
      this->body_->Translate(venv, tenv, level, label, errormsg);

  if (seq) {
    seq = tr::translateSeq(seq, body->exp_);
  } else {
    seq = body->exp_;
  }

  type::Ty *bodyTy = body->ty_;
  return new tr::ExpAndTy(seq, bodyTy);
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *ty = tenv->Look(this->typ_)->ActualTy();

  tr::Exp *size =
      this->size_->Translate(venv, tenv, level, label, errormsg)->exp_;
  tr::Exp *init =
      this->init_->Translate(venv, tenv, level, label, errormsg)->exp_;
  return new tr::ExpAndTy(tr::translateArray(size, init),
                          tenv->Look(this->typ_));
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<absyn::FunDec *> funDecList = this->functions_->GetList();
  // sym::Table<env::EnvEntry>* cur_venv = env::BaseVEnv();
  sym::Table<env::EnvEntry> *cur_venv = new sym::Table<env::EnvEntry>();

  for (auto funDec_it = funDecList.begin(); funDec_it != funDecList.end();
       funDec_it++) {

    type::TyList *formals =
        make_formal_tylist(tenv, (*funDec_it)->params_, errormsg);
    std::list<type::Ty *> formalTyList = formals->GetList();

    std::vector<bool> *argsBoolList = new std::vector<bool>();
    std::list<absyn::Field *> paraFieldList = (*funDec_it)->params_->GetList();
    auto paraField_it = paraFieldList.begin();
    for (auto formalTy_it = formalTyList.begin();
         formalTy_it != formalTyList.end(); formalTy_it++) {
      argsBoolList->push_back((*paraField_it)->escape_);
      paraField_it++;
    }

    temp::Label *name =
        temp::LabelFactory::NamedLabel((*funDec_it)->name_->Name());
    tr::Level *newLevel = tr::Level::NewLevel(level, name, argsBoolList);
    if ((*funDec_it)->result_) {
      type::Ty *resultTy = tenv->Look((*funDec_it)->result_);
      cur_venv->Enter((*funDec_it)->name_,
                      new env::FunEntry(newLevel, name, formals, resultTy));
      venv->Enter((*funDec_it)->name_,
                  new env::FunEntry(newLevel, name, formals, resultTy));
    } else {
      cur_venv->Enter(
          (*funDec_it)->name_,
          new env::FunEntry(newLevel, name, formals, type::VoidTy::Instance()));
      venv->Enter(
          (*funDec_it)->name_,
          new env::FunEntry(newLevel, name, formals, type::VoidTy::Instance()));
    }
  }

  for (auto funDec_it = funDecList.begin(); funDec_it != funDecList.end();
       funDec_it++) {
    type::TyList *formals =
        make_formal_tylist(tenv, (*funDec_it)->params_, errormsg);
    std::list<type::Ty *> formalTyList = formals->GetList();
    auto formalTy_it = formalTyList.begin();
    venv->BeginScope();
    env::EnvEntry *entry = venv->Look((*funDec_it)->name_);
    std::vector<tr::Access *> *accessList =
        ((env::FunEntry *)entry)->level_->Formals();

    int index = 1; // static link lies at index 0
    std::list<absyn::Field *> fieldList = (*funDec_it)->params_->GetList();
    for (auto field_it = fieldList.begin(); field_it != fieldList.end();
         field_it++, formalTy_it++) {
      venv->Enter((*field_it)->name_,
                  new env::VarEntry((*accessList)[index], (*formalTy_it)));
      index++;
    }
    tr::ExpAndTy *body =
        (*funDec_it)
            ->body_->Translate(venv, tenv, ((env::FunEntry *)entry)->level_,
                               ((env::FunEntry *)entry)->label_,
                               errormsg); //Δ label or entry->label?
    type::Ty *bodyTy = body->ty_;
    venv->EndScope();
    tr::translateFunctionDec(body->exp_, ((env::FunEntry *)entry)->level_);
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *init =
      this->init_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *initTy = init->ty_->ActualTy();
  if (!this->typ_ && typeid(*initTy) == typeid(type::NilTy)) {
    errormsg->Error(this->pos_,
                    "init should not be nil without type specified");
  } else if (this->typ_ && typeid(*initTy) == typeid(type::VoidTy)) {
    initTy = tenv->Look(this->typ_)->ActualTy();
  }
  tr::Access *access = tr::Access::allocLocal(level, this->escape_);
  venv->Enter(this->var_, new env::VarEntry(access, initTy));
  return new tr::NxExp(new tree::MoveStm(
      tr::translateSimpleVar(access, level)->UnEx(), init->exp_->UnEx()));
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<NameAndTy *> nameAndTylist = this->types_->GetList();
  sym::Table<type::Ty> *cur_tenv = new sym::Table<type::Ty>();

  for (auto nameTy_it = nameAndTylist.begin(); nameTy_it != nameAndTylist.end();
       nameTy_it++) {
    cur_tenv->Enter((*nameTy_it)->name_,
                    new type::NameTy((*nameTy_it)->name_, nullptr));
    tenv->Enter((*nameTy_it)->name_,
                new type::NameTy((*nameTy_it)->name_, nullptr));
  }

  for (auto nameTy_it = nameAndTylist.begin(); nameTy_it != nameAndTylist.end();
       nameTy_it++) {
    type::NameTy *ty = (type::NameTy *)tenv->Look((*nameTy_it)->name_);
    ty->ty_ = (*nameTy_it)->ty_->Translate(tenv, errormsg);
  }

  for (auto nameTy_it = nameAndTylist.begin(); nameTy_it != nameAndTylist.end();
       nameTy_it++) {
    type::Ty *cur = tenv->Look((*nameTy_it)->name_);
    type::Ty *ty = cur;
    while (typeid(*ty) == typeid(type::NameTy)) {
      ty = ((type::NameTy *)ty)->ty_;
    }
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::NameTy(this->name_, tenv->Look(this->name_));
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::FieldList *record =
      make_fieldlist(this->pos_, tenv, this->record_, errormsg);
  return new type::RecordTy(record);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::ArrayTy(tenv->Look(this->array_));
}

} // namespace absyn
