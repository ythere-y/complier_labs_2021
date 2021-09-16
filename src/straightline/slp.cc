#include "straightline/slp.h"

#include <iostream>

namespace A {

int A::CompoundStm::MaxArgs() const {
  assert(stm1 != nullptr && stm2 != nullptr);
  return stm1->MaxArgs() + stm2->MaxArgs();
}
Table *A::CompoundStm::Interp(Table *t) const {
  assert(stm1 != nullptr && stm2 != nullptr && t != nullptr);
  return stm2->Interp(stm1->Interp(t));
}

int A::AssignStm::MaxArgs() const {
  assert(exp != nullptr);
  return exp->MaxArgs() + 1;
}
Table *A::AssignStm::Interp(Table *t) const {
  assert(t != nullptr && exp != nullptr);
  IntAndTable *mid = exp->Interp(t);
  mid->t->Update(id, mid->i);
  return mid->t;
}

int A::PrintStm::MaxArgs() const {
  assert(exps != nullptr);
  return exps->MaxArgs();
}

Table *A::PrintStm::Interp(Table *t) const {
  assert(exps != nullptr && t != nullptr);
  IntAndTable *mid = exps->Interp(t);
  return mid->t;
}

int A::IdExp::MaxArgs() const {
  assert(id != "");
  return 1;
}
IntAndTable *A::IdExp::Interp(Table *t) const {
  assert(id != "" && t != nullptr);
  IntAndTable *res = new IntAndTable(t->Lookup(id), t);
  return res;
}

int A::NumExp::MaxArgs() const { return 1; }
IntAndTable *A::NumExp::Interp(Table *t) const {
  assert(t != nullptr);
  IntAndTable *res = new IntAndTable(num, t);
  return res;
}
int A::OpExp::MaxArgs() const {
  assert(left != nullptr && right != nullptr);
  return 1;
}
IntAndTable *A::OpExp::Interp(Table *t) const {
  assert(t != nullptr && left != nullptr && right != nullptr);
  IntAndTable *mid = left->Interp(t);
  int value_left = mid->i;
  mid = right->Interp(mid->t);
  int value_right = mid->i;
  switch (oper) {
  case PLUS:
    mid->i = value_left + value_right;
    break;
  case MINUS:
    mid->i = value_left - value_right;
    break;
  case TIMES:
    mid->i = value_left * value_right;
    break;
  case DIV:
    mid->i = value_left / value_right;
    break;
  }
  return mid;
}

int A::EseqExp::MaxArgs() const {
  assert(stm != nullptr && exp != nullptr);
  return stm->MaxArgs() + exp->MaxArgs();
}
IntAndTable *A::EseqExp::Interp(Table *t) const {
  assert(stm != nullptr && exp != nullptr && t != nullptr);
  return exp->Interp(stm->Interp(t));
}

int A::PairExpList::MaxArgs() const {
  assert(exp != nullptr && tail != nullptr);
  return exp->MaxArgs() + tail->MaxArgs();
}
IntAndTable *A::PairExpList::Interp(Table *t) const {
  assert(exp != nullptr && tail != nullptr && t != nullptr);
  return tail->Interp(exp->Interp(t)->t);
}

int A::LastExpList::MaxArgs() const {
  assert(exp != nullptr);
  return exp->MaxArgs();
}
IntAndTable *A::LastExpList::Interp(Table *t) const {
  assert(exp != nullptr && t != nullptr);
  return exp->Interp(t);
}

int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
} // namespace A
