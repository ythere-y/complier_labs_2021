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
#define LOG(format, args...)                                                   \
  do {                                                                         \
    FILE *debug_log = fopen("tiger.log", "a+");                                \
    fprintf(debug_log, "%d,%s: ", __LINE__, __func__);                         \
    fprintf(debug_log, format, ##args);                                        \
    fclose(debug_log);                                                         \
  } while (0)

#define COMMANLOG(format, level, label)                                        \
  do {                                                                         \
    LOG((format),                                                              \
        temp::LabelFactory::LabelString((level)->frame_->label_).c_str(),      \
        temp::LabelFactory::LabelString((label)).c_str());                     \
  } while (0)

extern frame::Frags *frags;

extern frame::RegManager *reg_manager;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  return new Access(level, level->frame_->allocLocal(escape));
}

class Cx {
public:
  temp::Label **trues_;
  temp::Label **falses_;
  // temp::Label **trues_;
  // temp::Label **falses_;
  tree::Stm *stm_;

  Cx(temp::Label **trues, temp::Label **falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

void fill_label(temp::Label **list, temp::Label *label) { list[0] = label; }

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

    tree::CjumpStm *stm = new tree::CjumpStm(
        tree::RelOp::NE_OP, exp_, new tree::ConstExp(0), nullptr, nullptr);

    temp::Label **trues = new temp::Label *[1];
    temp::Label **falses = new temp::Label *[1];
    trues[0] = stm->true_label_;
    falses[0] = stm->false_label_;
    // fill_label(trues,)
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
    printf("Error: can't change NxExp into Cx\n");
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
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    fill_label(cx_.trues_, t);
    fill_label(cx_.falses_, f);

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
    temp::Label *label = temp::LabelFactory::NewLabel();
    fill_label(cx_.trues_, label);
    fill_label(cx_.falses_, label);
    return new tree::SeqStm(cx_.stm_, new tree::LabelStm(label));
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};
void clearLog() {
  // 清空log内容
  std::ofstream fileout(
      "tiger.log",
      std::ios::trunc); // ios::trunc是清除原文件内容,可不写,默认就是它

  fileout.close();
}
void ProgTr::Translate() { /* TODO: Put your lab5 code here */
  // 准备生成最外层的frame
  // TODO:最主translate函数
  printf("hello");
  FillBaseVEnv();
  FillBaseTEnv();

  clearLog();

  // return;
  Level *mainframe = new Level(
      new frame::X64Frame(temp::LabelFactory::NamedLabel("tigermain"), nullptr),
      nullptr);
  temp::Label *mainlabel = temp::LabelFactory::NamedLabel("tigermain");
  // 开始翻译
  ExpAndTy *mainexp = absyn_tree_->Translate(
      venv_.get(), tenv_.get(), mainframe, mainlabel, errormsg_.get());
}

tree::Exp *findStaticLink(tr::Level *target, tr::Level *level) {

  tree::Exp *staticlink = new tree::TempExp(reg_manager->FramePointer());
  while (level != target) {
    staticlink = (*(level->frame_->fromals_->begin()))->ToExp(staticlink);
    staticlink = new tree::MemExp(staticlink,
                                  new tree::ConstExp(reg_manager->WordSize()));
    level = level->parent_;
  }
  return staticlink;
}
} // namespace tr

namespace absyn {
tr::Exp *TranslateNilExp() { return new tr::ExExp(new tree::ConstExp(0)); }
tr::Exp *TranslateSimpleVar(tr::Access *access, tr::Level *level) {
  // 翻译单个的简单值，要去找到staticlink
  tree::Exp *staticlink = tr::findStaticLink(access->level_, level);
  staticlink = access->access_->ToExp(staticlink);
  return new tr::ExExp(staticlink);
}

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  LOG("tree started\n");
  root_->Translate(venv, tenv, level, label, errormsg);
  LOG("tree finished\n");
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

#ifdef test
  LOG("Translate SimpleVar level %s label %s[name = %s]\n",
      temp::LabelFactory::LabelString(level->frame_->label_).c_str(),
      temp::LabelFactory::LabelString(label).c_str(), sym_->Name().c_str());
#endif
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::IntTy::Instance();
  env::EnvEntry *entry = venv->Look(sym_);

  if (!entry || DIFF(entry, env::VarEntry))
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().c_str());

  env::VarEntry *var_entry = (env::VarEntry *)entry;
  exp = TranslateSimpleVar(var_entry->access_, level);
  ty = var_entry->ty_->ActualTy();

  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
/* TODO: Put your lab5 code here */
#ifdef test
  LOG("Translate FieldVar level %s label %s\n",
      temp::LabelFactory::LabelString(level->frame_->label_).c_str(),
      temp::LabelFactory::LabelString(label).c_str());
#endif
  tr::ExpAndTy *check_var = var_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *real_ty = check_var->ty_->ActualTy();
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::IntTy::Instance();
  if (typeid(real_ty) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  } else {
    type::FieldList *fields = ((type::RecordTy *)real_ty)->fields_;
    int order = 0;
    auto get_fi = fields->GetList();
    for (auto it_fi = get_fi.begin(); it_fi != get_fi.end(); it_fi++) {
      if ((*it_fi)->name_ == sym_) {
        if (typeid(check_var->exp_) != typeid(tr::ExExp)) {
          errormsg->Error(pos_, "Error: fieldVar's loc must be a expression");
        }
        check_var->exp_->UnEx();
        tr::Exp *exp = new tr::ExExp(new tree::MemExp(
            check_var->exp_->UnEx(),
            new tree::ConstExp(order * reg_manager->WordSize())));
        type::Ty *ty = (*it_fi)->ty_->ActualTy();
        break;
      }
    }
  }
  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().c_str());
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
/* TODO: Put your lab5 code here */
#ifdef test
  LOG("Translate SubscriptVar level %s label %s\n",
      temp::LabelFactory::LabelString(level->frame_->label_).c_str(),
      temp::LabelFactory::LabelString(label).c_str());
#endif

  tr::ExpAndTy *check_var = var_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::IntTy::Instance();
  if (typeid(check_var->ty_->ActualTy()) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "array type required");
    return new tr::ExpAndTy(exp, ty);
  }

  tr::ExpAndTy *check_subscript =
      subscript_->Translate(venv, tenv, level, label, errormsg);
  if (typeid(check_subscript->ty_->ActualTy()) != typeid(type::IntTy)) {
    errormsg->Error(pos_, "array index must be interger");
    return new tr::ExpAndTy(exp, ty);
  }

  if (DIFF(check_var->exp_, tr::ExExp) ||
      DIFF(check_subscript->exp_, tr::ExExp)) {
    errormsg->Error(
        pos_, "Error: subscriptVar's loc or subscript must be an expression");
  }

  exp = new tr::ExExp(new tree::MemExp(new tree::BinopExp(
      tree::BinOp::PLUS_OP, check_var->exp_->UnEx(),
      new tree::BinopExp(tree::BinOp::MUL_OP, check_subscript->exp_->UnEx(),
                         new tree::ConstExp(reg_manager->WordSize())))));
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
/* TODO: Put your lab5 code here */
#ifdef test
  LOG("Translate VarExp level %s label %s\n",
      temp::LabelFactory::LabelString(level->frame_->label_).c_str(),
      temp::LabelFactory::LabelString(label).c_str());
#endif
  return var_->Translate(venv, tenv, level, label, errormsg);
  if (SAME(var_, SimpleVar))
    return ((SimpleVar *)var_)->Translate(venv, tenv, level, label, errormsg);
  else if (SAME(var_, FieldVar))
    return ((FieldVar *)var_)->Translate(venv, tenv, level, label, errormsg);
  else if (SAME(var_, SubscriptVar))
    return ((SubscriptVar *)var_)
        ->Translate(venv, tenv, level, label, errormsg);
  assert(0);
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
#ifdef test
  COMMANLOG("Translate IntExp level %s label %s\n", level, label);
#endif
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)),
                          type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
/* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate StringExp level %s label %s\n", level, label);
#endif
  temp::Label *string_label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(string_label, str_));

  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(string_label)),
                          type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate CallExp level %s label %s\n", level, label);
#endif
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::VoidTy::Instance();

  //先将函数找到
  env::EnvEntry *entry = venv->Look(func_);
  if (!entry || DIFF(entry, env::FunEntry)) {
    errormsg->Error(pos_, "undefined function %s", func_->Name().c_str());
    return new tr::ExpAndTy(exp, ty);
  }

  env::FunEntry *fun_entry = (env::FunEntry *)entry;
  ty = fun_entry->result_;
  if (!ty)
    ty = type::VoidTy::Instance();

  tree::ExpList *list = new tree::ExpList();
  auto get_args = args_->GetList();
  auto get_formal = fun_entry->formals_->GetList();
  auto it_args = get_args.begin();
  auto it_formal = get_formal.begin();
  for (; it_args != get_args.end() && it_formal != get_formal.end();) {
    tr::ExpAndTy *check_arg =
        (*it_args)->Translate(venv, tenv, level, label, errormsg);
    if (!check_arg->ty_->IsSameType((*it_formal))) {
      errormsg->Error(pos_, "para type mismatch");
      return new tr::ExpAndTy(exp, ty);
    }
    list->Append(check_arg->exp_->UnEx());
    it_args++;
    it_formal++;
  }
  LOG("args over\n");
  if (it_formal != get_formal.end()) {
    errormsg->Error(pos_, "too little params in function %s",
                    this->func_->Name().c_str());
    return new tr::ExpAndTy(exp, ty);
  }
  if (it_args != get_args.end()) {
    errormsg->Error(pos_, "too many params in funcion %s",
                    this->func_->Name().c_str());
    return new tr::ExpAndTy(exp, ty);
  }
  if (!fun_entry->level_ || !fun_entry->level_->parent_) {
    // temp::Label *func_name = temp::LabelFactory::NamedLabel(func_->Name());
    // tree::NameExp *name_exp = new tree::NameExp(func_name);
    exp = new tr::ExExp(frame::externalCall(func_->Name(), list));
  } else {
    // exp = new TR::ExExp(new T::CallExp(
    //     new T::NameExp(func),
    //     new T::ExpList(StaticLink(fun_entry->level->parent, level), list)));
    tree::Exp *staticlink =
        tr::findStaticLink(fun_entry->level_->parent_, level);
    list->Insert(staticlink);
    tree::CallExp *call_exp = new tree::CallExp(new tree::NameExp(func_), list);

    exp = new tr::ExExp(call_exp);
  }
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
/* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate OpExp level %s label %s\n", level, label);
#endif
  tr::ExpAndTy *check_left =
      left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *check_right =
      right_->Translate(venv, tenv, level, label, errormsg);

  tree::CjumpStm *stm = nullptr;
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::IntTy::Instance();
  int count = 0;

  switch (oper_) {
  case Oper::PLUS_OP:
  case Oper::MINUS_OP:
  case Oper::TIMES_OP:
  case Oper::DIVIDE_OP: {
    if (DIFF(check_left->ty_, type::IntTy))
      errormsg->Error(left_->pos_, "integer required");
    if (DIFF(check_right->ty_, type::IntTy))
      errormsg->Error(right_->pos_, "integer required");

    tree::BinOp key;
    switch (oper_) {
    case Oper::PLUS_OP:
      key = tree::BinOp::PLUS_OP;
      break;
    case Oper::MINUS_OP:
      key = tree::BinOp::MINUS_OP;
      break;
    case Oper::TIMES_OP:
      key = tree::BinOp::MUL_OP;
      break;
    case Oper::DIVIDE_OP:
      key = tree::BinOp::DIV_OP;
      break;
    }
    exp = new tr::ExExp(new tree::BinopExp(key, check_left->exp_->UnEx(),
                                           check_right->exp_->UnEx()));
    break;
  }
  case Oper::LT_OP:
  case Oper::LE_OP:
  case Oper::GT_OP:
  case Oper::GE_OP:
  case Oper::NEQ_OP: {
    if (DIFF(check_left->ty_, type::IntTy) &&
        DIFF(check_left->ty_, type::StringTy))
      errormsg->Error(left_->pos_, "integer or string required");
    if (DIFF(check_right->ty_, type::IntTy) &&
        DIFF(check_right->ty_, type::StringTy))
      errormsg->Error(right_->pos_, "integer or string required");
    if (!check_left->ty_->IsSameType(check_right->ty_))
      errormsg->Error(pos_, "same type required");

    tree::CjumpStm *stm;
    tree::RelOp rel_key;

    switch (oper_) {
    case Oper::LT_OP:
      rel_key = tree::RelOp::LT_OP;
      break;
    case Oper::LE_OP:
      rel_key = tree::RelOp::LE_OP;
      break;
    case Oper::GT_OP:
      rel_key = tree::RelOp::GT_OP;
      break;
    case Oper::GE_OP:
      rel_key = tree::RelOp::GE_OP;
      break;
    case Oper::NEQ_OP:
      rel_key = tree::RelOp::NE_OP;
      break;
    }

    stm = new tree::CjumpStm(rel_key, check_left->exp_->UnEx(),
                             check_right->exp_->UnEx(), nullptr, nullptr);
    // TODO:如何不使用patchlist
    // std::vector<temp::Label *> *trues = new std::vector<temp::Label *>();
    // std::vector<temp::Label *> *falses = new std::vector<temp::Label *>();
    // trues->push_back(stm->true_label_);
    // falses->push_back(stm->false_label_);
    temp::Label **trues = &(stm->true_label_);
    temp::Label **falses = &(stm->false_label_);
    // TODO:有问题
    exp = new tr::CxExp(trues, falses, stm);
    break;
  }
  case Oper::EQ_OP:
    tree::CjumpStm *stm;
    if (SAME(check_left->ty_, type::StringTy)) {
      tree::ExpList *args = new tree::ExpList();
      args->Append(check_left->exp_->UnEx());
      args->Append(check_right->exp_->UnEx());
      stm = new tree::CjumpStm(tree::EQ_OP,
                               frame::externalCall("stringEqual", args),
                               new tree::ConstExp(1), nullptr, nullptr);
      LOG("got EQ and call string equal\n");
    } else {
      LOG("get EQ but into normal\n");
      stm = new tree::CjumpStm(tree::RelOp::EQ_OP, check_left->exp_->UnEx(),
                               check_right->exp_->UnEx(), nullptr, nullptr);
    }
    // std::vector<temp::Label *> *trues = new std::vector<temp::Label *>();
    // std::vector<temp::Label *> *falses = new std::vector<temp::Label *>();
    // trues->push_back(stm->true_label_);
    // falses->push_back(stm->false_label_);
    temp::Label **trues = &(stm->true_label_);
    temp::Label **falses = &(stm->false_label_);
    exp = new tr::CxExp(trues, falses, stm);
    break;
  }
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate RecordExp level %s label %s\n", level, label);
#endif
  type::Ty *ty = tenv->Look(typ_);
  tr::ExExp *exp = NULL;
  tree::ExpList *list = new tree::ExpList();

  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return new tr::ExpAndTy(exp, type::IntTy::Instance());
  }

  ty = ty->ActualTy();
  if (DIFF(ty, type::RecordTy)) {
    errormsg->Error(pos_, "not record type %s", typ_->Name().c_str());
    return new tr::ExpAndTy(exp, ty);
  }

  auto get_re = ((type::RecordTy *)ty)->fields_->GetList();
  auto get_ef = fields_->GetList();
  auto it_re = get_re.begin();
  auto it_ef = get_ef.begin();
  int count = 0;
  for (; it_re != get_re.end() && it_ef != get_ef.end();) {
    count++;
    tr::ExpAndTy *check_exp =
        (*it_ef)->exp_->Translate(venv, tenv, level, label, errormsg);
    if (!check_exp->ty_->IsSameType((*it_re)->ty_))
      errormsg->Error(this->pos_, "record type unmatched");

    list->Append(check_exp->exp_->UnEx());
    it_re++;
    it_ef++;
  }
  // 分配一块新的空间
  temp::Temp *reg = temp::TempFactory::NewTemp();
  tree::ExpList *inner_list = new tree::ExpList();
  inner_list->Append(new tree::ConstExp(count * reg_manager->WordSize()));
  // tree::CallExp *to = new tree::CallExp(
  //     new tree::NameExp(temp::LabelFactory::NamedLabel("allocRecord")),
  //     inner_list);

  tree::Stm *stm = new tree::MoveStm(new tree::TempExp(reg),
                                     frame::externalCall("allocRecord", list));

  count = 0;
  auto get_list = list->GetList();
  auto it_list = get_list.begin();
  for (; it_list != get_list.end(); it_list++) {
    tree::MemExp *from =
        new tree::MemExp(new tree::TempExp(reg),
                         new tree::ConstExp(count * reg_manager->WordSize()));
    tree::MoveStm *add_one = new tree::MoveStm(from, (*it_list));
    stm = new tree::SeqStm(stm, add_one);
    count++;
  }

  exp = new tr::ExExp(new tree::EseqExp(stm, new tree::TempExp(reg)));

  return new tr::ExpAndTy(exp, ty);
}
tr::Exp *TranslateSeqExp(tr::Exp *left, tr::Exp *right) {
  if (right)
    return new tr::ExExp(new tree::EseqExp(left->UnNx(), right->UnEx()));
  else
    return new tr::ExExp(
        new tree::EseqExp(left->UnNx(), new tree::ConstExp(0)));
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

#ifdef test
  COMMANLOG("Translate SeqExp level %s label %s\n", level, label);
#endif

  auto get_seq = seq_->GetList();
  tr::Exp *exp = TranslateNilExp();
  if (!get_seq.size())
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());

  tr::ExpAndTy *check_exp;
  for (auto it_seq = get_seq.begin(); it_seq != get_seq.end(); it_seq++) {
    check_exp = (*it_seq)->Translate(venv, tenv, level, label, errormsg);
    exp = TranslateSeqExp(exp, check_exp->exp_);
  }
  TAN;
  return new tr::ExpAndTy(exp, check_exp->ty_);
}

tr::Exp *TranslateAssignExp(tr::Exp *var, tr::Exp *exp) {
  return new tr::NxExp(new tree::MoveStm(var->UnEx(), exp->UnEx()));
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate AssignExp level %s label %s\n", level, label);
#endif

  if (!DIFF(var_, SimpleVar)) {
    env::EnvEntry *entry = venv->Look(((SimpleVar *)var_)->sym_);
    if (entry->readonly_) {
      errormsg->Error(pos_, "loop variable can't be assigned");
    }
  }

  tr::ExpAndTy *check_var = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *check_exp = exp_->Translate(venv, tenv, level, label, errormsg);
  if (!check_var->ty_->IsSameType(check_exp->ty_))
    errormsg->Error(pos_, "unmatched assign exp");

  tr::Exp *exp = TranslateAssignExp(check_var->exp_, check_exp->exp_);
  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

#ifdef test
  COMMANLOG("Translate IfExp level %s label %s\n", level, label);
#endif
  tr::ExpAndTy *check_test =
      test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *check_then =
      then_->Translate(venv, tenv, level, label, errormsg);

  tr::Exp *exp = nullptr;

  if (elsee_) {
    tr::ExpAndTy *check_elsee =
        elsee_->Translate(venv, tenv, level, label, errormsg);
    if (!check_then->ty_->IsSameType(check_elsee->ty_)) {
      errormsg->Error(pos_, "then exp and else exp type mismatch");
      return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
    }

    tr::Cx testc = check_test->exp_->UnCx(errormsg);
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *true_label = temp::LabelFactory::NewLabel();
    temp::Label *false_label = temp::LabelFactory::NewLabel();
    temp::Label *meeting = temp::LabelFactory::NewLabel();
    tr::fill_label(testc.trues_, true_label);
    tr::fill_label(testc.falses_, false_label);
    tree::EseqExp *total;
    TAN;
    FILE *out = fopen("test.out", "w+");
    fprintf(out, "hello");
    fclose(out);
    out = fopen("test.out", "w+");
    TAN;
    testc.stm_->Print(out, 0);
    fclose(out);
    TAN;
    total = new tree::EseqExp(
        testc.stm_,
        new tree::EseqExp(
            new tree::LabelStm(true_label),
            new tree::EseqExp(
                new tree::MoveStm(new tree::TempExp(r),
                                  check_then->exp_->UnEx()),
                new tree::EseqExp(
                    new tree::JumpStm(
                        new tree::NameExp(meeting),
                        new std::vector<temp::Label *>(1, meeting)),
                    new tree::EseqExp(
                        new tree::LabelStm(false_label),
                        new tree::EseqExp(
                            new tree::MoveStm(new tree::TempExp(r),
                                              check_elsee->exp_->UnEx()),
                            new tree::EseqExp(
                                new tree::JumpStm(
                                    new tree::NameExp(meeting),
                                    new std::vector<temp::Label *>(1, meeting)),
                                new tree::EseqExp(new tree::LabelStm(meeting),
                                                  new tree::TempExp(r)))))))));
    exp = new tr::ExExp(total);
  } else {
    if (DIFF(check_then->ty_, type::VoidTy)) {
      errormsg->Error(pos_, "if-then exp's body must produce no value");
      return new tr::ExpAndTy(exp, type::VoidTy::Instance());
    }

    tr::Cx testc = check_test->exp_->UnCx(errormsg);
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *true_label = temp::LabelFactory::NewLabel();
    temp::Label *false_label = temp::LabelFactory::NewLabel();
    temp::Label *meeting = temp::LabelFactory::NewLabel();
    tr::fill_label(testc.trues_, true_label);
    tr::fill_label(testc.falses_, false_label);

    exp = new tr::NxExp(new tree::SeqStm(
        testc.stm_,
        new tree::SeqStm(new tree::LabelStm(true_label),
                         new tree::SeqStm(check_then->exp_->UnNx(),
                                          new tree::LabelStm(false_label)))));
  }
  LOG("get here\n");

  return new tr::ExpAndTy(exp, check_then->ty_);
}
/*
 * test_label:
 *      if condition goto body_label else goto done_label
 * body_label:
 *      body
 * 			break -----------+
 *      goto test_label  |
 * done_label: <---------+
 */
tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate WhileExp level %s label %s\n", level, label);
#endif
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::VoidTy::Instance();

  temp::Label *done_label = temp::LabelFactory::NewLabel();
  tr::ExpAndTy *check_test =
      test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *check_body =
      body_->Translate(venv, tenv, level, done_label, errormsg);
  if (DIFF(check_test->ty_, type::IntTy)) {
    errormsg->Error(test_->pos_, "integer required");
    return new tr::ExpAndTy(exp, ty);
  }
  if (DIFF(check_body->ty_, type::VoidTy)) {
    errormsg->Error(body_->pos_, "while body must produce no value");
    return new tr::ExpAndTy(exp, ty);
  }

  temp::Label *test_label = temp::LabelFactory::NewLabel();
  temp::Label *body_label = temp::LabelFactory::NewLabel();
  tr::Cx condition = check_test->exp_->UnCx(errormsg);
  tr::fill_label(condition.trues_, body_label);
  tr::fill_label(condition.falses_, done_label);

  exp = new tr::NxExp(new tree::SeqStm(
      new tree::LabelStm(test_label),
      new tree::SeqStm(
          condition.stm_,
          new tree::SeqStm(
              new tree::LabelStm(body_label),
              new tree::SeqStm(
                  check_body->exp_->UnNx(),
                  new tree::SeqStm(
                      new tree::JumpStm(
                          new tree::NameExp(test_label),
                          new std::vector<temp::Label *>(1, test_label)),
                      new tree::LabelStm(done_label)))))));

  return new tr::ExpAndTy(exp, ty);
}
/*
 * let
 * 		var := lo
 * 		__limit_var__ := hi
 * in
 * 		while var <= __limit_var__
 * 				body
 * 				if var == __limit_var__
 * 						break
 * 				var := var+1
 * end
 */

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate ForExp level %s label %s\n", level, label);
#endif
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::VoidTy::Instance();

  tr::ExpAndTy *check_lo = lo_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *check_hi = hi_->Translate(venv, tenv, level, label, errormsg);
  if (DIFF(check_lo->ty_->ActualTy(), type::IntTy) ||
      DIFF(check_hi->ty_->ActualTy(), type::IntTy)) {
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
    return new tr::ExpAndTy(exp, ty);
  }

  venv->BeginScope();
  venv->Enter(var_, new env::VarEntry(tr::Access::AllocLocal(level, escape_),
                                      check_lo->ty_, escape_));
  tr::ExpAndTy *check_body =
      body_->Translate(venv, tenv, level, label, errormsg);

  if (DIFF(check_body->ty_, type::VoidTy)) {
    errormsg->Error(pos_, "for body must produce no value");
    return new tr::ExpAndTy(exp, ty);
  }
  venv->EndScope();

  /*
   * var:=lo
   * __limit_var__ := hi
   */
  DecList *front_dec =
      new DecList(new VarDec(0, var_, sym::Symbol::UniqueSymbol("int"), lo_));
  front_dec->Prepend(new VarDec(0, sym::Symbol::UniqueSymbol("__limit_var__"),
                                sym::Symbol::UniqueSymbol("int"), hi_));
  /*
   * var <= __limit_var__
   */
  OpExp *inner_test = new OpExp(
      0, Oper::LE_OP, new VarExp(0, new SimpleVar(0, var_)),
      new VarExp(0,
                 new SimpleVar(0, sym::Symbol::UniqueSymbol("__limit_var__"))));
  /*
   * body
   */
  ExpList *inner_body_list = new ExpList(body_);
  /*
   * if var == __limit_var__
   * then break
   */
  IfExp *inner_if = new IfExp(
      0,
      new OpExp(0, Oper::EQ_OP, new VarExp(0, new SimpleVar(0, var_)),
                new VarExp(0, new SimpleVar(0, sym::Symbol::UniqueSymbol(
                                                   "__limit_var__")))),
      new BreakExp(0), NULL);
  /*
   * var := var + 1
   */
  AssignExp *inner_add =
      new AssignExp(0, new SimpleVar(0, var_),
                    new OpExp(0, PLUS_OP, new VarExp(0, new SimpleVar(0, var_)),
                              new IntExp(0, 1)));
  inner_body_list->Prepend(inner_if);
  inner_body_list->Prepend(inner_add);

  WhileExp *hole_while =
      new WhileExp(0, inner_test, new SeqExp(0, inner_body_list));

  absyn::Exp *forexp_to_letexp = new absyn::LetExp(0, front_dec, hole_while);

  return forexp_to_letexp->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate BreakExp level %s label %s\n", level, label);
#endif

  tree::Stm *stm = new tree::JumpStm(new tree::NameExp(label),
                                     new std::vector<temp::Label *>(1, label));
  tr::Exp *exp = new tr::NxExp(stm);
  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate LetExp level %s label %s\n", level, label);
#endif
  tr::Exp *exp = nullptr;
  type::Ty *ty = type::VoidTy::Instance();
  tree::Exp *res = nullptr;

  // 开始新的一层
  LOG("translate the let\n");
  venv->BeginScope();
  tenv->BeginScope();
  tree::Stm *dec = nullptr;
  if (decs_) {
    auto get_dec = decs_->GetList();
    if (get_dec.size()) {
      auto it_dec = get_dec.begin();
      dec = (*it_dec)->Translate(venv, tenv, level, label, errormsg)->UnNx();
      it_dec++;
      // 遍历所有的declaration，生成一个seqstm
      for (; it_dec != get_dec.end(); it_dec++) {
        exp = (*it_dec)->Translate(venv, tenv, level, label, errormsg);
        dec = new tree::SeqStm(dec, exp->UnNx());
      }
    }
  }

  // 翻译body部分
  LOG("translate the body\n");
  tr::ExpAndTy *check_body =
      body_->Translate(venv, tenv, level, label, errormsg);
  venv->EndScope();
  tenv->EndScope();
  TAN;
  // TODO:这里删除了一个stm为空的可能
  // 将let部分和body部分整合
  res = new tree::EseqExp(dec, check_body->exp_->UnEx());
  // 最终整合为一个expstm
  dec = new tree::ExpStm(res);
  // TODO:这里删除了对main函数的判断
  // 放入frags
  if (level == nullptr || level->frame_ == nullptr) {
    BENULL;
  } else {
    NONULL;
  }
  frags->PushBack(new frame::ProcFrag(dec, level->frame_));
  TAN;
  exp = new tr::ExExp(res);
  ty = check_body->ty_->ActualTy();
  TAN;
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate ArrayExp level %s label %s\n", level, label);
#endif
  type::Ty *ty = tenv->Look(typ_)->ActualTy();
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  if (DIFF(ty, type::ArrayTy)) {
    errormsg->Error(pos_, "not array type ");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }

  tr::ExpAndTy *check_size =
      size_->Translate(venv, tenv, level, label, errormsg);
  if (DIFF(check_size->ty_, type::IntTy)) {
    errormsg->Error(pos_, "type of size expression should be int");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }

  tr::ExpAndTy *check_init =
      init_->Translate(venv, tenv, level, label, errormsg);
  if (!check_init->ty_->IsSameType(((type::ArrayTy *)ty)->ty_)) {
    errormsg->Error(pos_, "type mismatch");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  tree::ExpList *inner_list = new tree::ExpList();
  inner_list->Append(check_size->exp_->UnEx());
  inner_list->Append(check_init->exp_->UnEx());
  tr::Exp *exp = new tr::ExExp(new tree::CallExp(
      new tree::NameExp(temp::LabelFactory::NamedLabel("init_array")),
      inner_list));
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate VoidExp level %s label %s\n", level, label);
#endif
  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate FunctionDec level %s label %s\n", level, label);
#endif
  // 类型检查用的table
  env::VEnvPtr check_table = new sym::Table<env::EnvEntry>();

  auto get_funcs = functions_->GetList();
  auto it_funcs = get_funcs.begin();
  // 遍历所有的函数定义
  for (; it_funcs != get_funcs.end(); it_funcs++) {
    // 核对函数名称是否有重复
    if (check_table->Look((*it_funcs)->name_)) {
      errormsg->Error((*it_funcs)->pos_, "two functions have the same name");
      continue;
    }
    // 获取函数的信息-name-params
    // 随便加入记录
    check_table->Enter((*it_funcs)->name_, new env::EnvEntry(false));
    // 获取函数的具体信息
    type::TyList *formal_tys =
        (*it_funcs)->params_->MakeFormalTyList(tenv, errormsg);
    std::vector<bool> *escapes(0);
    // 建立新的Level
    tr::Level *new_level =
        new tr::Level(level, (*it_funcs)->name_, (*it_funcs)->params_);
    // 如果没有范数值，默认为void
    type::Ty *res = type::VoidTy::Instance();
    // 区分是否有返回值
    if ((*it_funcs)->result_) {
      res = tenv->Look((*it_funcs)->result_);
      if (!res) {
        errormsg->Error((*it_funcs)->pos_, "FunctionDec undefined result.");
        continue;
      }
    }
    // TODO:这里的类型需要重新审查
    // 在venv中记录返回值
    venv->Enter(
        (*it_funcs)->name_,
        new env::FunEntry(new_level, (*it_funcs)->name_, formal_tys, res));
  }

  // 第二次遍历函数
  it_funcs = get_funcs.begin();
  for (; it_funcs != get_funcs.end(); it_funcs++) {
    // 开始新层
    venv->BeginScope();

    FieldList *records = (*it_funcs)->params_;

    // 找出函数类型
    env::FunEntry *funentry = (env::FunEntry *)venv->Look((*it_funcs)->name_);
    // 得到参数信息
    type::TyList *tylist =
        (*it_funcs)->params_->MakeFormalTyList(tenv, errormsg);

    // 得到参数信息
    type::TyList *formal_typs = funentry->formals_;
    // 得到参数的获取方式(所有)
    std::vector<frame::Access *> *formal_accs =
        funentry->level_->frame_->fromals_;

    auto get_records = records->GetList();
    auto get_formal_types = formal_typs->GetList();
    //准备遍历每个参数，需要的3个数组信息
    auto it_formal_types = get_formal_types.begin();
    auto it_records = get_records.begin();
    auto it_formal_accs = formal_accs->begin();
    for (; it_records != get_records.end();) {
      venv->Enter(
          (*it_records)->name_,
          new env::VarEntry(new tr::Access(funentry->level_, *it_formal_accs),
                            *it_formal_types));
      it_formal_accs++;
      it_formal_types++;
      it_records++;
    }
    // 翻译body
    tr::ExpAndTy *entry = (*it_funcs)->body_->Translate(
        venv, tenv, funentry->level_, funentry->label_, errormsg);
    // 检查body部分的返回值
    if (!entry->ty_->IsSameType(type::VoidTy::Instance()) &&
        (*it_funcs)->result_ == nullptr)
      errormsg->Error((*it_funcs)->pos_, "procedure returns value");
    if ((*it_funcs)->result_ &&
        !entry->ty_->IsSameType(tenv->Look((*it_funcs)->result_)->ActualTy()))
      errormsg->Error((*it_funcs)->pos_,
                      "function return value type incorrect");
    // 结束层
    venv->EndScope();
    // 最后一句把结果移动到指定寄存器中
    tree::MoveStm *total_last = new tree::MoveStm(
        new tree::TempExp(reg_manager->ReturnValue()), entry->exp_->UnEx());
    frame::Frag *new_one =
        new frame::ProcFrag(total_last, funentry->level_->frame_);
    frags->PushBack(new_one);
  }
#ifdef test
  LOG("get here\n");
#endif
  return TranslateNilExp();
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate VarDec level %s label %s\n", level, label);
#endif

  tr::ExpAndTy *check_init =
      init_->Translate(venv, tenv, level, label, errormsg);
  tr::Access *access;
  if (!typ_) {
    if (!DIFF(check_init->ty_, type::NilTy))
      errormsg->Error(pos_, "init should not be nil without type specified");
  } else {
    type::Ty *ty = tenv->Look(typ_);
    if (SAME(check_init->ty_, type::NilTy) &&
        DIFF(ty->ActualTy(), type::RecordTy))
      errormsg->Error(pos_, "init should not be nil without type specified");
    if (ty && !ty->IsSameType(check_init->ty_))
      errormsg->Error(pos_, "type mismatch");
  }
  access = tr::Access::AllocLocal(level, true);
  venv->Enter(var_, new env::VarEntry(access, check_init->ty_));

  return TranslateAssignExp(TranslateSimpleVar(access, level),
                            check_init->exp_);
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  COMMANLOG("Translate TypeDec level %s label %s\n", level, label);
#endif
  auto get_types = types_->GetList();
  auto it_type = get_types.begin();
  for (; it_type != get_types.end(); it_type++) {
    auto sec_it = it_type;
    sec_it++;
    for (; sec_it != get_types.end(); sec_it++)
      if ((*it_type)->name_ == (*sec_it)->name_)
        errormsg->Error(pos_, "two types have the same name");
    tenv->Enter((*it_type)->name_,
                new type::NameTy((*it_type)->name_, nullptr));
  }

  it_type = get_types.begin();
  for (; it_type != get_types.end(); it_type++) {
    type::NameTy *name_ty = (type::NameTy *)tenv->Look((*it_type)->name_);
    name_ty->ty_ = (*it_type)->ty_->Translate(tenv, errormsg);
  }
  // TODO:可能会有问题
  //  再次遍历所有的属性看是否有循环
  bool hasCycle = false;
  it_type = get_types.begin();
  for (; it_type != get_types.end(); it_type++) {
    type::Ty *ty = tenv->Look((*it_type)->name_);
    //如果有名字类型
    if (!DIFF(ty, type::NameTy)) {
      type::Ty *tyTy = ((type::NameTy *)ty)->ty_;
      // 一直向下翻译查找
      while (!DIFF(tyTy, type::NameTy)) {
        type::NameTy *nameTy = (type::NameTy *)tyTy;
        //但凡中间有一个的名字与外层相同
        if (nameTy->sym_->Name() == (*it_type)->name_->Name()) {
          errormsg->Error(pos_, "illegal type cycle");
          hasCycle = true;
          break;
        }
        tyTy = nameTy->ty_;
      }
    }
    if (hasCycle)
      break;
  }
  return TranslateNilExp();
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

#ifdef test
  LOG("Translate NameTy\n");
#endif
  type::Ty *ty = tenv->Look(name_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", name_->Name().c_str());
    return type::VoidTy::Instance();
  }
  return new type::NameTy(name_, ty);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  LOG("Translate RecordTy\n");
#endif
  type::FieldList *fields = record_->MakeFieldList(tenv, errormsg);
  return new type::RecordTy(fields);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
#ifdef test
  LOG("Translate ArrayTy\n");
#endif
  type::Ty *ty = tenv->Look(array_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", array_->Name().c_str());
    return new type::ArrayTy(NULL);
  }
  return new type::ArrayTy(ty);
}

} // namespace absyn
