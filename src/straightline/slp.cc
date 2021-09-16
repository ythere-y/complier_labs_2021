#include "straightline/slp.h"

#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return stm1->MaxArgs() + stm2->MaxArgs();
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return stm2->Interp(stm1->Interp(t));
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs() + 1;
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *mid = exp->Interp(t);
  mid->t->Update(id, mid->i);
  return mid->t;
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exps->MaxArgs();
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *mid = exps->Interp(t);
  return mid->t;
}

int A::IdExp::MaxArgs() const {
  // TODO:
}
IntAndTable *A::IdExp::Interp(Table *t) const {
  IntAndTable *res = new IntAndTable(t->Lookup(id), t);
  return res;
}
int A::NumExp::MaxArgs() const {
  // TODO:
}
IntAndTable *A::NumExp::Interp(Table *t) const {
  IntAndTable *res = new IntAndTable(num, t);
  return res;
}
int A::OpExp::MaxArgs() const {
  // TODO:
  return left->MaxArgs() + right->MaxArgs();
}
IntAndTable *A::OpExp::Interp(Table *t) const {
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
  // TODO:
}
IntAndTable *A::EseqExp::Interp(Table *t) const {
  // TODO:
}
int A::PairExpList::MaxArgs() const { return exp->MaxArgs() + tail->MaxArgs(); }
IntAndTable *A::PairExpList::Interp(Table *t) const {
  return exp->Interp(tail->Interp(t)->t);
}
int A::LastExpList::MaxArgs() const { return exp->MaxArgs(); }
IntAndTable *A::LastExpList::Interp(Table *t) const { return exp->Interp(t); }

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
