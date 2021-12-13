#include "tiger/semant/semant.h"
#include "tiger/absyn/absyn.h"
bool test_on = false;
// bool test_on = true;
namespace absyn {

int loop_count = 0;
void test_type(type::Ty *typ_ty, int pos_, err::ErrorMsg *errormsg,
               std::string name = "") {
  if (typeid(*typ_ty) == typeid(type::Ty)) {
    errormsg->Error(pos_, (name + "  is a type").data());
  }
  if (typeid(*typ_ty) == typeid(type::ArrayTy)) {
    errormsg->Error(pos_, (name + "  is a arraytype").data());
  }
  if (typeid(*typ_ty) == typeid(type::NilTy)) {
    errormsg->Error(pos_, (name + "  is a niltype").data());
  }
  if (typeid(*typ_ty) == typeid(type::IntTy)) {
    errormsg->Error(pos_, (name + "  is a inttype").data());
  }
  if (typeid(*typ_ty) == typeid(type::StringTy)) {
    errormsg->Error(pos_, (name + "  is a stringtype").data());
  }
  if (typeid(*typ_ty) == typeid(type::NameTy)) {
    errormsg->Error(pos_, (name + "  is a nametype").data());
  }
}

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a simplevar[sym = %s]", sym_->Name().data());
  env::EnvEntry *entry = venv->Look(sym_);
  env::VarEntry *change = static_cast<env::VarEntry *>(entry);

  if (entry && typeid(*entry) == typeid(env::VarEntry)) {
    return (static_cast<env::VarEntry *>(entry))->ty_;
  } else {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
  }
  return type::IntTy::Instance();
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */

  // TODO:可能为空
  type::Ty *var_ty =
      var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (typeid(*var_ty) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "var is not a record type");
    return type::IntTy::Instance();
  }

  type::FieldList *list = (static_cast<type::RecordTy *>(var_ty))->fields_;
  // TODO:待商榷
  auto list_it = list->GetList().begin();
  for (; list_it != list->GetList().end(); list_it++) {
    if ((*list_it)->name_ == this->sym_)
      return (*list_it)->ty_;
  }
  errormsg->Error(pos_, "field %s doesn't exist", this->sym_->Name().data());
  return type::VoidTy::Instance();
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *var_ty =
      var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *exp_ty = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (typeid(*var_ty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "array type required");
    return type::IntTy::Instance();
  }

  if (typeid(*exp_ty) != typeid(type::IntTy)) {
    errormsg->Error(pos_, "array index must be int");
    return type::IntTy::Instance();
  }

  return (static_cast<type::ArrayTy *>(var_ty))->ActualTy();
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a varexp");

  if (typeid(*var_) == typeid(SimpleVar)) {
    if (test_on)
      printf("it's simplevar\n");
    return (static_cast<SimpleVar *>(var_))
        ->SemAnalyze(venv, tenv, labelcount, errormsg);
  } else if (typeid(*var_) == typeid(FieldVar)) {
    if (test_on)
      printf("it's fieldvar\n");
    return (static_cast<FieldVar *>(var_))
        ->SemAnalyze(venv, tenv, labelcount, errormsg);
  } else if (typeid(*var_) == typeid(SubscriptVar)) {
    if (test_on)
      printf("it's subscriptvar\n");
    return (static_cast<SubscriptVar *>(var_))
        ->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a nilexp");
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a intexp");
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a stringexp");
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a callexp");
  env::EnvEntry *entry = venv->Look(func_);
  if (!entry || typeid(*entry) != typeid(env::FunEntry)) {
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    return type::IntTy::Instance();
  }

  type::TyList *formals = (static_cast<env::FunEntry *>(entry))->formals_;
  auto args_it = args_->GetList().begin();
  auto formals_it = formals->GetList().begin();
  for (; formals_it != formals->GetList().end(); formals_it++) {
    // 先判断args是否可以用，顺便判断数量是否相等
    if (args_it == args_->GetList().end()) {
      errormsg->Error(pos_, "too little params in function %s",
                      func_->Name().data());
      return type::IntTy::Instance();
    }
    // 主要就是判断args和formals的类型是否相同
    type::Ty *arg_ty = (*args_it)->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!arg_ty->IsSameType((*formals_it))) {
      errormsg->Error(pos_, "para type mismatch");
      return type::IntTy::Instance();
    }
    args_it++;
  }
  // 数量上还需要判断是否是args太多
  if (args_it != args_->GetList().end()) {
    errormsg->Error(pos_, "too many params in function %s",
                    func_->Name().data());
  }
  return (static_cast<env::FunEntry *>(entry))->result_->ActualTy();
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a opexp");
  type::Ty *left_ty =
      left_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *right_ty =
      right_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (oper_ == absyn::PLUS_OP || oper_ == absyn::MINUS_OP ||
      oper_ == absyn::TIMES_OP || oper_ == absyn::DIVIDE_OP) {
    //如果是加减乘除，只能对int进行操作
    if (typeid(*left_ty) != typeid(type::IntTy))
      errormsg->Error(left_->pos_, "integer required");
    if (typeid(*right_ty) != typeid(type::IntTy))
      errormsg->Error(right_->pos_, "integer required");
    return type::IntTy::Instance();
  }
  if (!left_ty->IsSameType(right_ty)) {
    errormsg->Error(pos_, "same type required");
    return type::IntTy::Instance();
  }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //从table中找出这个类
  if (test_on) {

    errormsg->Error(pos_, "in a recordexp");
    errormsg->Error(pos_, "[type : %s]", typ_->Name().data());
  }
  type::Ty *typ_ty = tenv->Look(typ_);

  if (!typ_ty) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return type::IntTy::Instance();
  }

  if (test_on)
    test_type(typ_ty, pos_, errormsg, "typ_ty");

  typ_ty = typ_ty->ActualTy();
  if (typeid(*typ_ty) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not record type %s", typ_->Name().data());
    return type::IntTy::Instance();
  }
  // 代码中写的fields的类型，需要从a转为ty
  auto fields_it = fields_->GetList().begin();
  //从table中的类记录中读取包含的变量的类型
  type::FieldList *records = (static_cast<type::RecordTy *>(typ_ty))->fields_;
  auto records_it = records->GetList().begin();
  //开始遍历所有的属性
  for (; fields_it != fields_->GetList().end() &&
         records_it != records->GetList().end();) {
    //计算表达式的结果类型
    type::Ty *field_ty =
        (*fields_it)
            ->exp_->SemAnalyze(venv, tenv, labelcount, errormsg)
            ->ActualTy();
    //赋值顺序必须相同
    if ((*fields_it)->name_ != (*records_it)->name_) {
      errormsg->Error(pos_, "field not defined");
      return type::IntTy::Instance();
    }
    //类型必须相同
    if (typeid(*field_ty) != typeid(*(*records_it)->ty_)) {
      errormsg->Error(pos_, "field type mismatch");
      return type::IntTy::Instance();
    }
    fields_it++;
    records_it++;
  }
  // 检查属性数量是否相同
  if (fields_it != fields_->GetList().end() ||
      records_it != records->GetList().end()) {
    errormsg->Error(pos_, "field amount mismatch");
    return type::IntTy::Instance();
  }
  return typ_ty;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a seqexp");
  auto seq_it = seq_->GetList().begin();
  type::Ty *res_ty;

  for (; seq_it != seq_->GetList().end(); seq_it++) {
    res_ty = (*seq_it)->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  return res_ty;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(0, "in an assignexp");
  if (typeid(*var_) == typeid(SimpleVar)) {
    env::EnvEntry *entry = venv->Look(static_cast<SimpleVar *>(var_)->sym_);
    if (entry && entry->readonly_) {
      errormsg->Error(pos_, "loop variable can't be assigned");
      return type::VoidTy::Instance();
    }
  }

  type::Ty *var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *exp_ty = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!var_ty->IsSameType(exp_ty))
    errormsg->Error(pos_, "unmatched assign exp");
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(0, "in an ifexp");
  type::Ty *test_ty =
      test_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (typeid(*test_ty) != typeid(type::IntTy)) {

    errormsg->Error(pos_, "integer required");
    return type::IntTy::Instance();
  }

  type::Ty *then_ty = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (elsee_ != nullptr) {
    type::Ty *else_ty = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (then_ty->ActualTy()->IsSameType(else_ty->ActualTy())) {
      return then_ty;
    } else {
      errormsg->Error(pos_, "then exp and else exp type mismatch");
      return type::VoidTy::Instance();
    }
  } else {
    if (typeid(*then_ty) != typeid(type::VoidTy)) {
      errormsg->Error(pos_, "if-then exp's body must produce no value");
      return type::IntTy::Instance();
    }
    return type::VoidTy::Instance();
  }
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a whileexp");
  type::Ty *test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  loop_count++;
  type::Ty *body_ty = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  loop_count--;
  if (typeid(*test_ty) != typeid(type::IntTy)) {
    errormsg->Error(test_->pos_, "integer required");
    return type::IntTy::Instance();
  }
  if (typeid(*body_ty) != typeid(type::VoidTy)) {
    errormsg->Error(body_->pos_, "while body must produce no value");
    return type::IntTy::Instance();
  }
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a forexp");
  loop_count++;
  type::Ty *lo_ty = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *hi_ty = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (typeid(*lo_ty) != typeid(type::IntTy))
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
  if (typeid(*hi_ty) != typeid(type::IntTy))
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");

  venv->BeginScope();
  tenv->BeginScope();
  // errormsg->Error(0, "get here");
  venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));
  type::Ty *body_ty = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  venv->EndScope();
  tenv->EndScope();
  loop_count--;
  if (test_on)
    errormsg->Error(pos_, "out a forexp");
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a breakexp");
  if (!loop_count)
    errormsg->Error(pos_, "break is not inside any loop");
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();
  if (test_on)
    errormsg->Error(pos_, "in a letexp");
  int time = 0;
  int end_time = 0;
  auto dec_it = decs_->GetList().begin();
  for (; dec_it != decs_->GetList().end(); dec_it++) {
    if (test_on)
      errormsg->Error(pos_, "start time : %d", time++);

    (*dec_it)->SemAnalyze(venv, tenv, labelcount, errormsg);

    if (test_on)
      errormsg->Error(pos_, "end time : %d", end_time++);
  }
  type::Ty *result;
  if (test_on)
    errormsg->Error(pos_, "get into the body");
  if (!body_)
    result = type::VoidTy::Instance();
  else
    result = body_->SemAnalyze(venv, tenv, labelcount, errormsg);

  tenv->EndScope();
  venv->EndScope();
  return result;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a arrayexp, [type : %s]", typ_->Name().data());

  type::Ty *typ_ty = tenv->Look(typ_);

  if (typ_ty == nullptr) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return type::VoidTy::Instance()->ActualTy();
  }
  typ_ty = typ_ty->ActualTy();
  if (typeid(*typ_ty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "not an array type");
    return type::IntTy::Instance();
  }
  type::Ty *size_ty =
      size_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *init_ty =
      init_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (typeid(*size_ty) != typeid(type::IntTy)) {
    errormsg->Error(pos_, "required int for size");
    return type::IntTy::Instance();
  }
  type::Ty *inside_ty = (static_cast<type::ArrayTy *>(typ_ty))->ty_;
  // test_type(init_ty, pos_, errormsg, "init_ty");
  // test_type(typ_ty, pos_, errormsg, "typ_ty");
  // test_type(inside_ty, pos_, errormsg, "inside_ty");
  if (!init_ty->IsSameType(inside_ty)) {
    errormsg->Error(pos_, "type mismatch");
    return type::IntTy::Instance();
  }
  return typ_ty;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a funciondec");
  //第一次遍历，把名字全都放入
  auto function_it = functions_->GetList().begin();
  for (; function_it != functions_->GetList().end(); function_it++) {

    type::Ty *result_ty = type::VoidTy::Instance();
    if ((*function_it)->result_)
      result_ty = tenv->Look((*function_it)->result_);

    absyn::FieldList *params = (*function_it)->params_;
    type::TyList *formals = params->MakeFormalTyList(tenv, errormsg);

    if (venv->Look((*function_it)->name_))
      errormsg->Error((*function_it)->pos_, "two functions have the same name");
    else {
      //把函数的定义放入
      venv->Enter((*function_it)->name_, new env::FunEntry(formals, result_ty));
    }
  }
  //第二次遍历,需要放入函数的body
  function_it = functions_->GetList().begin();
  for (; function_it != functions_->GetList().end(); function_it++) {

    venv->BeginScope();
    // 将函数的参数加入到变量定义中
    absyn::FieldList *params = (*function_it)->params_;
    type::TyList *formals = params->MakeFormalTyList(tenv, errormsg);
    auto formal_it = formals->GetList().begin();
    auto param_it = params->GetList().begin();
    for (; param_it != params->GetList().end(); formal_it++, param_it++)
      venv->Enter((*param_it)->name_, new env::VarEntry(*formal_it));

    //开始解析函数的body
    type::Ty *bodyTy =
        (*function_it)->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    type::Ty *decTy =
        ((env::FunEntry *)venv->Look((*function_it)->name_))->result_;

    if (!bodyTy->IsSameType(decTy)) {
      if (typeid(*decTy) == typeid(type::VoidTy))
        errormsg->Error((*function_it)->body_->pos_, "procedure returns value");
      else
        errormsg->Error((*function_it)->body_->pos_, "return type mismatch");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a vardec");

  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typ_ == nullptr) {
    if (typeid(*init_ty) == typeid(type::NilTy))
      errormsg->Error(pos_, "init should not be nil without type specified");
    if (test_on)
      printf("Enter var it[name=%s] [type = none]\n", var_->Name().data());
    venv->Enter(var_, new env::VarEntry(init_ty));
  } else {
    type::Ty *typ_ty = tenv->Look(typ_);
    if (typ_ty == nullptr) {
      errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
      return;
    }
    if (typ_ty->IsSameType(init_ty)) {
      if (test_on)
        printf("Enter var it[name=%s] [type = %s]\n", var_->Name().data(),
               typ_->Name().data());
      venv->Enter(var_, new env::VarEntry(tenv->Look(typ_)));
    } else
      errormsg->Error(pos_, "type mismatch");
  }
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a typedec");

  auto type = (types_->GetList()).begin();
  //第一次遍历，先把head加入
  for (; type != types_->GetList().end(); type++) {
    if (tenv->Look((*type)->name_)) {
      errormsg->Error(pos_, "two types have the same name");
    } else {
      tenv->Enter((*type)->name_, new type::NameTy((*type)->name_, NULL));
    }
  }
  type = (types_->GetList()).begin();
  //第二次遍历，读取body的内容，开始为之前的null赋值
  for (; type != types_->GetList().end(); type++) {
    if (test_on) {
      printf("in the second time [name = %s]\n", (*type)->name_->Name().data());
      type::Ty *test_get = (*type)->ty_->SemAnalyze(tenv, errormsg);
      test_type(test_get, pos_, errormsg, "test_get");
    }
    type::Ty *ty = tenv->Look((*type)->name_);
    (static_cast<type::NameTy *>(ty))->ty_ =
        (*type)->ty_->SemAnalyze(tenv, errormsg);
  }

  type = (types_->GetList()).begin();
  //第三次遍历，检查非法的定义循环
  for (; type != types_->GetList().end(); type++) {
    type::NameTy *yuan =
        static_cast<type::NameTy *>(tenv->Look((*type)->name_));
    type::NameTy *cur = static_cast<type::NameTy *>(yuan->ty_);
    while (typeid(*cur) == typeid(type::NameTy) && cur != yuan) {
      cur = static_cast<type::NameTy *>(cur->ty_);
    }
    if (cur == yuan) {
      errormsg->Error(pos_, "illegal type cycle");
      break;
    }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a nametype [name = %s]", name_->Name().data());
  type::Ty *name_ty = tenv->Look(name_);
  if (!name_ty) {
    errormsg->Error(pos_, "undefined type %s", name_->Name().data());
    return type::VoidTy::Instance();
  }
  return name_ty;
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a recordtype");
  auto record_it = record_->GetList().begin();
  type::FieldList *get = new type::FieldList();
  for (; record_it != record_->GetList().end(); record_it++) {
    if (test_on)
      printf("finding [name = %s]\t[type = %s]\n",
             (*record_it)->name_->Name().data(),
             (*record_it)->typ_->Name().data());
    type::Ty *typ_ty = tenv->Look((*record_it)->typ_);
    if (typ_ty == nullptr) {
      errormsg->Error((*record_it)->pos_, "undefined type %s",
                      (*record_it)->typ_->Name().data());
    }
    type::Field *add_one = new type::Field((*record_it)->name_, typ_ty);
    get->Append(add_one);
  }
  return new type::RecordTy(get);
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (test_on)
    errormsg->Error(pos_, "in a arraytype [array : %s]", array_->Name().data());
  type::Ty *array_ty = tenv->Look(array_);
  // test_type(array_ty, 0, errormsg);
  if (!array_ty) {
    errormsg->Error(pos_, "undefined type %s", array_->Name().data());
    return type::VoidTy::Instance();
  }
  return new type::ArrayTy(array_ty);
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace sem
