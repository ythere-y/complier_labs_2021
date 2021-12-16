#include "tiger/semant/semant.h"
#include "tiger/absyn/absyn.h"
namespace absyn {

int loop_count = 0;

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
    env::EnvEntry *entry = venv->Look(sym_);
    if(entry && typeid(*entry) == typeid(env::VarEntry))
      return ((env::VarEntry *)entry)->ty_->ActualTy();
    else {
      errormsg->Error(pos_, "undefined variable %s", sym_->Name().c_str());
      return type::IntTy::Instance();
    }
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*ty) != typeid(type::RecordTy)){
    errormsg->Error(pos_, "not a record type");
    return type::IntTy::Instance();
  }
  type::FieldList *fields = ((type::RecordTy *)ty)->fields_;
  std::list<type::Field *> fieldList = fields->GetList();
  for(auto it=fieldList.begin(); it!=fieldList.end(); it++)
  {
    if((*it)->name_ == sym_) 
      return (*it)->ty_; 
  }
  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().c_str());
  return type::IntTy::Instance(); 
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *exp_ty = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*var_ty) != typeid(type::ArrayTy)){
    errormsg->Error(pos_, "array type required");
    return type::IntTy::Instance();
  }
  if(typeid(*exp_ty) != typeid(type::IntTy)){
    errormsg->Error(pos_, "array index must be integer");
    return type::IntTy::Instance();
  }
  return ((type::ArrayTy *)var_ty)->ty_->ActualTy();
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(typeid(*var_) == typeid(SimpleVar))
    return ((SimpleVar *)var_)->SemAnalyze(venv, tenv, labelcount, errormsg);
  else if(typeid(*var_) == typeid(FieldVar))
    return ((FieldVar *)var_)->SemAnalyze(venv, tenv, labelcount, errormsg);
  else if(typeid(*var_) == typeid(SubscriptVar))
    return ((SubscriptVar *)var_)->SemAnalyze(venv, tenv, labelcount, errormsg);
  else 
    assert(0);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry *entry = venv->Look(func_);
  if(!entry || typeid(*entry) != typeid(env::FunEntry)){
    errormsg->Error(pos_, "undefined function %s", func_->Name().c_str());
    return type::IntTy::Instance();
  }

  type::TyList *formals = ((env::FunEntry *)entry)->formals_;
  std::list<type::Ty *> TyList = formals->GetList();
  std::list<Exp *> argsList = args_->GetList();
  auto ty_it = TyList.begin();
  auto arg_it = argsList.begin();
  for( ; ty_it != TyList.end() && arg_it != argsList.end(); ty_it++, arg_it++)
  {
    type::Ty *ty = (*arg_it)->SemAnalyze(venv, tenv, labelcount, errormsg);
    if(!ty->IsSameType(*ty_it)){
      errormsg->Error(pos_, "para type mismatch");
      return type::IntTy::Instance();
    }
  }
  if(ty_it != TyList.end())
  {
    errormsg->Error(pos_, "too few params in function %s", this->func_->Name().c_str());
    return type::IntTy::Instance();
  }
  if(arg_it != argsList.end())
  {
    errormsg->Error(pos_, "too many params in function %s", this->func_->Name().c_str());
    // return type::IntTy::Instance();
  }
  return ((env::FunEntry *)entry)->result_->ActualTy();
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *left_ty = left_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *right_ty = right_->SemAnalyze(venv, tenv, labelcount, errormsg);
  switch (oper_)
  {
  case Oper::PLUS_OP:
  case Oper::MINUS_OP:
  case Oper::TIMES_OP:
  case Oper::DIVIDE_OP:
    if(typeid(*left_ty) != typeid(type::IntTy))
      errormsg->Error(left_->pos_, "integer required");
    if(typeid(*right_ty) != typeid(type::IntTy))
      errormsg->Error(right_->pos_, "integer required");
    break;
 
  case Oper::LE_OP:
  case Oper::LT_OP:
  case Oper::GE_OP:
  case Oper::GT_OP:
    if(typeid(*left_ty) != typeid(type::IntTy) && typeid(*left_ty) != typeid(type::StringTy))
      errormsg->Error(pos_, "integer or string required");
    if(typeid(*right_ty) != typeid(type::IntTy) && typeid(*right_ty) != typeid(type::StringTy))
      errormsg->Error(pos_, "integer or string required");
    if(!left_ty->IsSameType(right_ty))
      errormsg->Error(pos_, "same type required");
    break;

  case Oper::EQ_OP:
  case Oper::NEQ_OP:
    if((typeid(*left_ty) == typeid(type::RecordTy) | (typeid(*left_ty) == typeid(type::ArrayTy))) &&
      typeid(*right_ty) == typeid(type::NilTy)) {
      break;
    }
    if((typeid(*right_ty) == typeid(type::RecordTy) | (typeid(*left_ty) == typeid(type::ArrayTy))) &&
      typeid(*left_ty) == typeid(type::NilTy)) {
      break;
    }
    if(typeid(*left_ty) != typeid(type::IntTy) && typeid(*left_ty) != typeid(type::StringTy)
      && typeid(*left_ty) != typeid(type::RecordTy) && typeid(*left_ty) != typeid(type::ArrayTy))
      errormsg->Error(left_->pos_, "integer, string, record or array required");
    if(typeid(*right_ty) != typeid(type::IntTy) && typeid(*right_ty) != typeid(type::StringTy)
      && typeid(*right_ty) != typeid(type::RecordTy) && typeid(*right_ty) != typeid(type::ArrayTy))
      errormsg->Error(right_->pos_, "integer, string, record or array required");
    if(typeid(*left_ty) == typeid(type::NilTy) && typeid(*right_ty) == typeid(type::NilTy))
      errormsg->Error(this->pos_, "at least one operand should not be Nil");
    if(!left_ty->IsSameType(right_ty)
      && !(typeid(*left_ty) == typeid(type::RecordTy) && typeid(*right_ty) == typeid(type::NilTy)))
      errormsg->Error(pos_, "same type required");
    break;
  default:
    assert(0);
  }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *ty = tenv->Look(typ_);    //Δ why search for this
  if(!ty){
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return type::IntTy::Instance();
  }
  ty = ty->ActualTy();
  if(typeid(*ty) != typeid(type::RecordTy)){
    errormsg->Error(pos_, "not record type %s", typ_->Name().c_str());
    return type::IntTy::Instance();
  }

  absyn::EFieldList *Fields = this->fields_;
  type::FieldList *Records = ((type::RecordTy *)ty)->fields_;
  std::list<EField *> FieldsList = Fields->GetList();
  std::list<type::Field *> RecordsList = Records->GetList();
  auto field_it = FieldsList.begin();
  auto record_it = RecordsList.begin();
  for(; field_it != FieldsList.end() && record_it != RecordsList.end(); field_it++, record_it++)
  {
    type::Ty *field_ty = (*field_it)->exp_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
    if((*field_it)->name_ != (*record_it)->name_){
      errormsg->Error(this->pos_, "field not defined");
      return type::IntTy::Instance();
    }
    if(typeid(*field_ty) != typeid(*((*record_it)->ty_)) && typeid(*((*record_it)->ty_)) != typeid(type::NameTy)) {
      errormsg->Error(this->pos_, "field type mismatch");
      return type::IntTy::Instance();
    }
  }
  if(field_it != FieldsList.end() || record_it != RecordsList.end()){
    errormsg->Error(this->pos_, "field amount mismatch");
    return type::IntTy::Instance();
  }
  return ty; 
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<Exp *> seq_expList = seq_->GetList();
  if(seq_expList.size() == 0)
    return type::VoidTy::Instance();
  type::Ty *ty;
  for(auto seq_it = seq_expList.begin(); seq_it != seq_expList.end(); seq_it++)
  {
    ty = (*seq_it)->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  return ty;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (typeid(*var_) == typeid(SimpleVar)) {
    env::EnvEntry *entry = venv->Look(static_cast<SimpleVar *>(var_)->sym_);
    if (entry && entry->readonly_) {
      errormsg->Error(pos_, "loop variable can't be assigned");
      // return type::IntTy::Instance();
      return type::VoidTy::Instance();
    }
  }

  type::Ty *var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *exp_ty = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!var_ty->IsSameType(exp_ty))
    errormsg->Error(pos_, "unmatched assign exp");
  // return var_ty;
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(typeid(*(this->test_->SemAnalyze(venv, tenv, labelcount, errormsg))) != typeid(type::IntTy)){
    errormsg->Error(this->pos_, "integer required");
    return type::IntTy::Instance();
  }
  type::Ty *then_ty = this->then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(this->elsee_ == nullptr){
    if(typeid(*then_ty) != typeid(type::VoidTy)){
      errormsg->Error(this->pos_, "if-then exp's body must produce no value");
      return type::IntTy::Instance();
    }
    return type::VoidTy::Instance();
  } else {
    if(then_ty->ActualTy()->IsSameType(this->elsee_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy())){
      return then_ty;
    } else {
      errormsg->Error(this->pos_, "then exp and else exp type mismatch");
      return type::VoidTy::Instance();
    }
  }
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  loop_count++;
  type::Ty *body_ty = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  loop_count--;
  if(typeid(*test_ty) != typeid(type::IntTy))
    errormsg->Error(test_->pos_, "integer required");
  if(typeid(*body_ty) != typeid(type::VoidTy))
    errormsg->Error(body_->pos_, "while body must produce no value");
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  loop_count++;
  type::Ty *lo_ty = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *hi_ty = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*lo_ty) != typeid(type::IntTy))
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
  if(typeid(*hi_ty) != typeid(type::IntTy))
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
  venv->BeginScope();
  tenv->BeginScope();
  venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));
  type::Ty *body_ty = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  venv->EndScope();
  tenv->EndScope();
  loop_count--;
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (!loop_count)
    errormsg->Error(pos_, "break is not inside any loop");
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();
  std::list<Dec *> decList = decs_->GetList();
  for(auto it = decList.begin(); it != decList.end(); it++){
    (*it)->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  type::Ty *ty = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  venv->EndScope();
  tenv->EndScope();
  return ty;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *ty = tenv->Look(typ_)->ActualTy();
  if(ty == nullptr) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return type::VoidTy::Instance();
  }
  if(typeid(*size_->SemAnalyze(venv, tenv,labelcount, errormsg)) != typeid(type::IntTy)){
    errormsg->Error(pos_, "size should be integer");
    return type::VoidTy::Instance();
  }
  if(typeid(*ty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "not array type");
    return type::VoidTy::Instance();
  }
  type::ArrayTy *arrayTy = (type::ArrayTy *)ty;
  if(!init_->SemAnalyze(venv, tenv, labelcount, errormsg)->IsSameType(arrayTy->ty_)) {
    errormsg->Error(pos_, "type mismatch");
    return type::VoidTy::Instance();
  }
  return arrayTy;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<FunDec *> funList = functions_->GetList();
  for(auto it = funList.begin(); it != funList.end(); it++){
    type::Ty *result_ty = type::VoidTy::Instance();
    if((*it)->result_)
      result_ty = tenv->Look((*it)->result_);
    if (venv->Look((*it)->name_))
      errormsg->Error((*it)->pos_, "two functions have the same name");
    else {
      venv->Enter((*it)->name_, new env::FunEntry((*it)->params_->MakeFormalTyList(tenv, errormsg), result_ty));
    }
  }

  for(auto it = funList.begin(); it != funList.end(); it++){
    venv->BeginScope();
    std::list<Field *> paramsList= (*it)->params_->GetList(); 
    for(auto iter = paramsList.begin(); iter != paramsList.end(); iter++){
      type::Ty *ty = tenv->Look((*iter)->typ_);
      if(ty == nullptr)
        errormsg->Error((*iter)->pos_, "undefined type %s", (*iter)->typ_->Name().c_str());
      venv->Enter((*iter)->name_, new env::VarEntry(ty));
    }
    type::Ty *bodyTy = (*it)->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    type::Ty *decTy = ((env::FunEntry *)venv->Look((*it)->name_))->result_;
    if(!bodyTy->IsSameType(decTy)) {
      if(typeid(*decTy) == typeid(type::VoidTy))
        errormsg->Error((*it)->body_->pos_, "procedure returns value");
      else 
        errormsg->Error((*it)->body_->pos_, "return type mismatch");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */

  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typ_ == nullptr) {
    if (typeid(*init_ty) == typeid(type::NilTy))
      errormsg->Error(pos_, "init should not be nil without type specified");
    venv->Enter(var_, new env::VarEntry(init_ty));
  } else {
    type::Ty *ty = tenv->Look(typ_);
    if (ty == nullptr) {
      errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
      return;
    }
    if (ty->IsSameType(init_ty)) {
      venv->Enter(var_, new env::VarEntry(tenv->Look(typ_)));
    } else
      errormsg->Error(pos_, "type mismatch");
  }
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<NameAndTy *> typeList =  types_->GetList();
  for(auto it = typeList.begin(); it != typeList.end(); it++){
    if (tenv->Look((*it)->name_)) {
      errormsg->Error(pos_, "two types have the same name");
    } else {
      tenv->Enter((*it)->name_, new type::NameTy((*it)->name_, NULL));
    }
  }
  for(auto it2 = typeList.begin(); it2 != typeList.end(); it2++){
    type::NameTy *nameTy = (type::NameTy *)tenv->Look((*it2)->name_);
    nameTy->ty_ = (*it2)->ty_->SemAnalyze(tenv, errormsg);
  }
  
  bool hasCycle = false;
  for(auto it3 = typeList.begin(); it3 != typeList.end(); it3++){
    type::Ty *ty = tenv->Look((*it3)->name_);
    if(typeid(*ty) == typeid(type::NameTy)){
      type::Ty *tyTy = ((type::NameTy *)ty)->ty_;
      while(typeid(*tyTy) == typeid(type::NameTy)){
        type::NameTy *nameTy = (type::NameTy *) tyTy;
        if(nameTy->sym_->Name() == (*it3)->name_->Name()) {
          errormsg->Error(pos_, "illegal type cycle");
          hasCycle = true;
          break;
        }
        tyTy = nameTy->ty_;
      }
    }
    if(hasCycle)
      break;
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *ty = tenv->Look(name_);
  if(!ty){
    errormsg->Error(pos_, "undefined type %s", name_->Name().c_str());
    return type::VoidTy::Instance();
  }
  return new type::NameTy(name_, ty);
}


type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::FieldList *fields = record_->MakeFieldList(tenv, errormsg);
  return new type::RecordTy(fields);
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *ty = tenv->Look(array_);
  if(!ty){
    errormsg->Error(pos_, "undefined type %s", array_->Name().c_str());
    return type::VoidTy::Instance();
  }
  return new type::ArrayTy(ty);
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace tr
