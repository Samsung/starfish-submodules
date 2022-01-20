{% if strict_mode %}
{% import 'util.cpp' as util_macro %}
{%- call util_macro.ifdef(function.flags) %}
static ValueRef* {{ function.name }}Function(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    STARFISH_UNIMPLEMENTED("Unimplemented function \"{{ function.name }}\" in {{ name }}");
    return ValueRef::createUndefined();
}
{%- endcall %}
{% endif %}
