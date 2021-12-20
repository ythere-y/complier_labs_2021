#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {

bool test_mode = false;

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  /* TODO: Put your lab5 code here */
  this->root_->Traverse(env, 1);
}

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  esc::EscapeEntry *entry = env->Look(this->sym_);
  if (depth > entry->depth_) {
    if(test_mode)
      printf("************ %d, %d", depth, entry->depth_);
    *(entry->escape_) = true;
  }
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
  this->subscript_->Traverse(env, depth);
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
}

void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

}

void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

}

void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

}

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<Exp*> expList = this->args_->GetList();
  for(auto it=expList.begin(); it!=expList.end(); it++) {
    (*it)->Traverse(env, depth);
  }
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->left_->Traverse(env, depth);
  this->right_->Traverse(env, depth);
}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<EField *> efieldList = this->fields_->GetList();
  for(auto it=efieldList.begin(); it!=efieldList.end(); it++) {
    (*it)->exp_->Traverse(env, depth);
  }
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<Exp *> expList = this->seq_->GetList();
  for(auto it=expList.begin(); it!=expList.end(); it++) {
    (*it)->Traverse(env, depth);
  }
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
  this->exp_->Traverse(env, depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->test_->Traverse(env, depth);
  this->then_->Traverse(env, depth);
  if(this->elsee_)
    this->elsee_->Traverse(env, depth);
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if(test_mode)
    printf("enter a WhileExp");

  this->test_->Traverse(env, depth);
  this->body_->Traverse(env, depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->escape_ = false;
  env->Enter(this->var_, new esc::EscapeEntry (depth, &(this->escape_)));
  this->lo_->Traverse(env, depth);
  this->hi_->Traverse(env, depth);
  this->body_->Traverse(env, depth);
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

}

void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  if(test_mode)
    printf("enter a LetExp");

  std::list<Dec *>  decList = this->decs_->GetList();
  for(auto it=decList.begin(); it!=decList.end(); it++) {
    (*it)->Traverse(env, depth);
  }
  this->body_->Traverse(env, depth);
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->size_->Traverse(env, depth);
  this->init_->Traverse(env, depth);
}

void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

}

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<FunDec *> funDecList = this->functions_->GetList();
  for(auto it=funDecList.begin(); it!=funDecList.end(); it++) {
    env->BeginScope();
    std::list<Field *> paramList = (*it)->params_->GetList();
    for(auto iter=paramList.begin(); iter!=paramList.end(); iter++) {
      (*iter)->escape_ = false;
      env->Enter((*iter)->name_, new esc::EscapeEntry (depth+1, &((*iter)->escape_)));
    }
    (*it)->body_->Traverse(env, depth+1);
    env->EndScope();
  }
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  if(test_mode)
    printf("enter a VarDec");

  this->escape_ = false;
  env->Enter(this->var_, new esc::EscapeEntry (depth, &(this->escape_)));
  this->init_->Traverse(env, depth);
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

}

} // namespace absyn
