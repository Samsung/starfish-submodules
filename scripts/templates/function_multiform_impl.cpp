{% import 'util.cpp' as util_macro %}
{% set skip_type_check = True %}
{% for fn in function.operations %}
{% if fn.conditions|length > 0 %}
{% call util_macro.ifdef(fn.flags) %}
// Strong type checker (Check types for all arguments if possible)
static bool {{ '%s%s'|format(fn.name, fn.id) }}Checker(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    {% for argidx in fn.conditions %}
    ValueRef* arg{{argidx}} = argv[{{argidx}}];
    if (!{{ util_macro.gen_check_type(fn.arguments[argidx].type, 'arg%d'|format(argidx))|trim }}) {
        return false;
    }
    {% endfor %}
    return true;
}
{% if fn.strong_condition_count > 0 and fn.strong_condition_count != fn.conditions|length %}
// Weak type checker (Check types for only strong types)
static bool {{ '%s%sWeak'|format(fn.name, fn.id) }}Checker(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    {% for argidx in fn.conditions %}
        {% if fn.arguments[argidx].type.kind in strong_type_kinds %}
    ValueRef* arg{{argidx}} = argv[{{argidx}}];
    if (!{{ util_macro.gen_check_type(fn.arguments[argidx].type, 'arg%d'|format(argidx))|trim }}) {
        return false;
    }
        {% endif %}
    {% endfor %}
    return true;
}
{% endif %}
{% endcall %}
{% endif %}
{% set function = fn %}
{% include 'function_impl.cpp' ignore missing %}
{% endfor %}
static ValueRef* {{ function.name }}Function(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    size_t argCount = argc;
    {% set hasCondition = [] %}
    // NOTE Some of conditions below never be reached
    {% for fn in function.operations %}
        {% set cond = 'false' %}
        {% if fn.conditions|length > 0 %}
            {% set sigfn = ' && %s%sChecker(state, thisValue, argc, argv, isNewExpression)'|format(fn.name, fn.id) %}
{% call util_macro.ifdef(fn.flags) %}
    if (argCount >= {{fn.min_passed_count}}{{sigfn}}) {
        {{ 'return %s%sFunction(state, thisValue, argc, argv, isNewExpression);'|format(fn.name, fn.id) }}
    }
{% endcall %}
        {% else %}
            {% set cond = 'true' %}
        {% endif %}
        {% set hasCondition = hasCondition.append(cond) %}
    {% endfor %}
    {% set hasStrongCondition = [] %}
    {% for fn in function.operations %}
        {% set cond = 'false' %}
        {% if fn.conditions|length > 0 and fn.strong_condition_count != fn.conditions|length %}
            {% if fn.strong_condition_count > 0 %}
                {% set sigfn = ' && %s%sWeakChecker(state, thisValue, argc, argv, isNewExpression)'|format(fn.name, fn.id) %}
{% call util_macro.ifdef(fn.flags) %}
    if (argCount >= {{fn.min_passed_count}}{{sigfn}}) {
        {{ 'return %s%sFunction(state, thisValue, argc, argv, isNewExpression);'|format(fn.name, fn.id) }}
    }
{% endcall %}
            {% else %}
                {% set cond = 'true' %}
            {% endif %}
        {% endif %}
        {% set hasStrongCondition = hasStrongCondition.append(cond) %}
    {% endfor %}
    {% for fn in function.operations %}
        {% if hasCondition[loop.index-1] == 'true' or hasStrongCondition[loop.index-1] == 'true' %}
            {% set sigfn = '' %}
{% call util_macro.ifdef(fn.flags) %}
    if (argCount >= {{fn.min_passed_count}}{{sigfn}}) {
        {{ 'return %s%sFunction(state, thisValue, argc, argv, isNewExpression);'|format(fn.name, fn.id) }}
    }
{% endcall %}
        {% endif %}
    {% endfor %}
    COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "{{ fnname }}", "{{ name }}", SIGNATURE_NOT_FOUND);
    THROW_EXCEPTION(msg);
}


