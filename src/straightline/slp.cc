#include "straightline/slp.h"

#include <iostream>

namespace A {

int A::CompoundStm::MaxArgs() const {
  // assert(stm1 != nullptr && stm2 != nullptr);
  int front = stm1->MaxArgs();
  int back = stm2->MaxArgs();
  if (front >= back)
    return front;
  else
    return back;
}
int A::AssignStm::MaxArgs() const {

  // assert(exp != nullptr);
  return exp->MaxArgs(0);
}
int A::PrintStm::MaxArgs() const {
  // assert(exps != nullptr);
  return exps->MaxArgs(1);
}
int A::IdExp::MaxArgs(int cur) const {
  // assert(id != "");
  return cur;
}
int A::NumExp::MaxArgs(int cur) const { return cur; }
int A::OpExp::MaxArgs(int cur) const {
  // assert(left != nullptr && right != nullptr);
  return cur;
}

int A::EseqExp::MaxArgs(int cur = 1) const {
  assert(stm != nullptr && exp != nullptr);
  int inner = stm->MaxArgs();
  if (inner >= cur)
    return inner;
  else
    return cur;
}
int A::PairExpList::MaxArgs(int cur) const {
  assert(exp != nullptr && tail != nullptr);
  int front = exp->MaxArgs(cur);
  int back = tail->MaxArgs(cur + 1);
  if (front >= back)
    return front;
  else
    return back;
}
int A::LastExpList::MaxArgs(int cur) const {
  assert(exp != nullptr);
  int get = exp->MaxArgs(cur);
  if (get >= cur)
    return get;
  else
    return cur;
}

Table *A::CompoundStm::Interp(Table *t) const {
  Table *front = stm1->Interp(t);

  return stm2->Interp(front);
}
Table *A::AssignStm::Interp(Table *t) const {

  IntAndTable *mid = exp->Interp(t);
  mid->t = mid->t->Update(id, mid->i);
  return mid->t;
}
Table *A::PrintStm::Interp(Table *t) const {
  assert(exps != nullptr && t != nullptr);
  IntAndTable *mid = exps->Interp(t);
  return mid->t;
}

IntAndTable *A::OpExp::Interp(Table *t) const {
  assert(left != nullptr && right != nullptr);

  IntAndTable *mid = left->Interp(t);
  int value_left = mid->i;
  int value_right = 0;
  mid = right->Interp(mid->t);
  value_right = mid->i;

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
IntAndTable *A::IdExp::Interp(Table *t) const {
  assert(id != "");
  IntAndTable *res = new IntAndTable(t->Lookup(id), t);
  return res;
}
IntAndTable *A::NumExp::Interp(Table *t) const {
  IntAndTable *res = new IntAndTable(num, t);
  return res;
}
IntAndTable *A::EseqExp::Interp(Table *t) const {
  assert(stm != nullptr && exp != nullptr);

  Table *front = stm->Interp(t);
  IntAndTable *res = exp->Interp(front);
}

IntAndTable *A::PairExpList::Interp(Table *t) const {
  assert(exp != nullptr && tail != nullptr);
  IntAndTable *front = exp->Interp(t);
  printf("%d ", front->i);
  return tail->Interp(front->t);
}

IntAndTable *A::LastExpList::Interp(Table *t) const {
  assert(exp != nullptr);
  IntAndTable *get = exp->Interp(t);
  printf("%d\n", get->i);
  return get;
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
