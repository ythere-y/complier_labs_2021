#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {
bool test_on = false;
void trans(esc::EscEnvPtr env, int depth, sym::Symbol *sym) {
  if (test_on)
    printf("try to trans [name = %s]\n", sym->Name().c_str());
  esc::EscapeEntry *look_get = env->Look(sym);
  if (look_get == nullptr) {
    if (test_on)
      printf("did'n find it\n");
    return;
  }
  if (test_on)
    printf("fond it, [depth = %d] [look_depth = %d] \n", depth,
           look_get->depth_);
  // 如果在下层被调用了
  if (depth > look_get->depth_) {
    *(look_get->escape_) = true;
    if (test_on)
      printf("change happened\n");
  }
}

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  /* TODO: Put your lab5 code here */
  root_->Traverse(env, 1);
}

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a simplevar\n");
  trans(env, depth, sym_);
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on) {

    printf("get into a field\n");
    if (var_ == nullptr)
      printf("****** var is null\n");
    if (sym_ == nullptr)
      printf("****** sym is null\n");
  }
  var_->Traverse(env, depth);
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a subscripvar\n");
  var_->Traverse(env, depth);
  subscript_->Traverse(env, depth);
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a varexp \n");
  var_->Traverse(env, depth);
}

void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on) {
    printf("get into a call\n");
    if (args_ == nullptr)
      printf("**** args is null\n");
  }
  std::list<Exp *> get_list = args_->GetList();
  for (auto it = get_list.begin(); it != get_list.end(); it++) {
    (*it)->Traverse(env, depth);
  }
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a op\n");
  left_->Traverse(env, depth);
  right_->Traverse(env, depth);
}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a record\n");
  std::list<EField *> get_list = fields_->GetList();
  for (auto it = get_list.begin(); it != get_list.end(); it++)
    (*it)->exp_->Traverse(env, depth);
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a seq\n");
  std::list<Exp *> get_list = seq_->GetList();
  for (auto it = get_list.begin(); it != get_list.end(); it++)
    (*it)->Traverse(env, depth);
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a assign\n");
  var_->Traverse(env, depth);
  exp_->Traverse(env, depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a if\n");
  test_->Traverse(env, depth);
  then_->Traverse(env, depth);
  if (elsee_)
    elsee_->Traverse(env, depth);
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a while\n");
  test_->Traverse(env, depth);
  body_->Traverse(env, depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a for\n");
  escape_ = false;
  env->Enter(var_, new esc::EscapeEntry(depth, &escape_));
  lo_->Traverse(env, depth);
  hi_->Traverse(env, depth);
  body_->Traverse(env, depth);
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a break\n");
}

void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  std::list<Dec *> get_list = decs_->GetList();
  if (test_on)
    printf("get into a let\n");
  for (auto it = get_list.begin(); it != get_list.end(); it++)
    (*it)->Traverse(env, depth);
  if (test_on) {
    printf("start body\n");
    if (body_ == nullptr)
      printf("******body is null\n");
  }
  body_->Traverse(env, depth);
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a array\n");
  size_->Traverse(env, depth);
  init_->Traverse(env, depth);
}

void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
}

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if (test_on)
    printf("get into a func dec\n");
  std::list<FunDec *> get_fun_list = functions_->GetList();
  for (auto it_fun = get_fun_list.begin(); it_fun != get_fun_list.end();
       it_fun++) {
    env->BeginScope();
    std::list<Field *> get_field_list = (*it_fun)->params_->GetList();
    for (auto it_par = get_field_list.begin(); it_par != get_field_list.end();
         it_par++) {
      (*it_par)->escape_ = false;
      env->Enter((*it_par)->name_,
                 new esc::EscapeEntry(depth + 1, &(*it_par)->escape_));
    }
    (*it_fun)->body_->Traverse(env, depth + 1);
    env->EndScope();
  }
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  if (test_on)
    printf("get into a vardec\n");
  escape_ = false;
  env->Enter(var_, new esc::EscapeEntry(depth, &escape_));
  if (test_on)
    if (init_ == nullptr)
      printf("*******the init is null\n");
  init_->Traverse(env, depth);
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
}

} // namespace absyn
