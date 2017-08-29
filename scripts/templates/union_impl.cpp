extern {{ union_item }} to{{ union_item }}FromValueRef(ExecutionStateRef* state, ValueRef* from);
extern ValueRef* toValueRefFrom{{ union_item }}(ExecutionStateRef* state, {{ union_item }}& from);
extern bool is{{ union_item }}(ExecutionStateRef* state, ValueRef* from);


