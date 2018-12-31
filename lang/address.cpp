#include "address.h"
#include "structural_node.h"
#include "program.h"

TLANG_NAMESPACE_BEGIN


// This part should be generated by the compiler
// The tree is built in a bottom-up order, so that upper level can learn about
// the size etc. of children.

/*
using placeholder_u = real;
using placeholder_v = real;
using node1 = forked<placeholder_u, placeholder_v>;
using root = fixed<32, node1>;

root r;

// Global look_up functions that take a data structure instance, and an index
// bit mixing is done here.
real &look_up_u(root &r, int i, int j, int k) {
  auto &n1 = r.look_up(i * 32 + j);
  auto &n2 = n1.get<1>();
  return n2;
}
*/

TLANG_NAMESPACE_END
