

#undef MAKE_STACK_BOX
#undef GET_ASSIGNMENT

#define MAKE_STACK_BOX(c, type, size) box<type> c((type*)alloca(sizeof(type) * (size)), (size))

// Now requires bool flag to be declared before the macro is used.
#define GET_ASSIGNMENT(c, constraint)                                                              \
  const size_t num_vars##c = constraint->getVarsSingleton()->size();                             \
  box<pair<SysInt, DomainInt>> c(                                                                  \
      (pair<SysInt, DomainInt>*)(alloca(sizeof(pair<SysInt, DomainInt>) * num_vars##c * 2)),       \
      num_vars##c * 2);                                                                            \
  flag = constraint->getSatisfyingAssignment(c);
