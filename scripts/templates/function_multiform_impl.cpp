{% import 'util.cpp' as util_macro %}
{% set skip_type_check = True %}
{% for fn in function.operations %}
    {% if fn.conditions|length > 0 %}
{% call util_macro.ifdef(fn.flags) %}
static bool {{ '%s%s'|format(fn.name, fn.id) }}Checker(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
        {% for argidx in fn.conditions %}
    ValueRef* arg{{argidx}} = argv[{{argidx}}];
            {% if fn.arguments[argidx].type.kind == 'Typeref' %}
    if (!_CHECK_TYPEOF(arg{{argidx}}, {{fn.arguments[argidx].type.name}})) {
        return false;
    }
            {% endif %}
        {% endfor %}
    return true;
}
{% endcall %}
    {% endif %}
    {% set function = fn %}
{% include 'function_impl.cpp' ignore missing %}
{% endfor %}
static ValueRef* {{ function.name }}Function(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    size_t argCount = argc;
    {% for fn in function.operations %}
        {% if fn.conditions|length > 0 %}
            {% set sigfn = ' && %s%sChecker(state, thisValue, argc, argv, isNewExpression)'|format(fn.name, fn.id) %}
        {% else %}
            {% set sigfn = '' %}
        {% endif %}
{% call util_macro.ifdef(fn.flags) %}
    if (argCount >= {{fn.min_passed_count}}{{sigfn}}) {
        {{ 'return %s%sFunction(state, thisValue, argc, argv, isNewExpression);'|format(fn.name, fn.id) }}
    }
{% endcall %}
    {% endfor %}
    COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "{{ fnname }}", "{{ name }}", SIGNATURE_NOT_FOUND);
    THROW_EXCEPTION(msg);
}


