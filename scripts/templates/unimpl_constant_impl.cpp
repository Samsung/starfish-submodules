{% if strict_mode %}
{% import 'util.cpp' as util_macro %}
{%- call util_macro.ifdef(constant.flags) %}
static ValueRef* {{ constant.name }}Unimplemented(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    STARFISH_BINDING_ASSERT_UNIMPLEMENTED("Unimplemented constant \"{{ constant.name }}\" in {{ name }}\n");
    return ValueRef::createUndefined();
}
{%- endcall %}
{% endif %}
