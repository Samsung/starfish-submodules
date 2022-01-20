{% if strict_mode %}
// Implement for constructor
static ValueRef* {{ name|lower }}Constructor(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    STARFISH_UNIMPLEMENTED("Unimplemented constructor \"{{ name }}\"");
    return ValueRef::createUndefined();
}
{% endif %}