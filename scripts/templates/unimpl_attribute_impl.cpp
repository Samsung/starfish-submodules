{% if strict_mode %}
{% import 'util.cpp' as util_macro %}
{%- call util_macro.ifdef(attribute.flags) %}
static ValueRef* {{ attribute.name }}UnimplementedFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    STARFISH_BINDING_ASSERT_UNIMPLEMENTED("Unimplemented attribute \"{{ attribute.name }}\" in {{ name }}\n");
    return ValueRef::createUndefined();
}
{% endcall %}
{% endif %}
