{% import 'util.cpp' as util_macro %}
{% set skip_type_check = True %}
{% for fn in function.operations %}
    {% if fn.conditions|length > 0 %}
{% call util_macro.ifdef(fn.flags) %}
static bool {{ '%s%s'|format(fn.name, fn.id) }}Checker(ESVMInstance* instance)
{
        {% for argidx in fn.conditions %}
    ESValue arg{{argidx}} = instance->currentExecutionContext()->readArgument({{argidx}});
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
static ESValue {{ function.name }}Function(ESVMInstance* instance)
{
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    {% for fn in function.operations %}
        {% if fn.conditions|length > 0 %}
            {% set sigfn = ' && %s%sChecker(instance)'|format(fn.name, fn.id) %}
        {% else %}
            {% set sigfn = '' %}
        {% endif %}
{% call util_macro.ifdef(fn.flags) %}
    if (argCount >= {{fn.min_passed_count}}{{sigfn}}) {
        {{ 'return %s%sFunction(instance);'|format(fn.name, fn.id) }}
    }
{% endcall %}
    {% endfor %}
    THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_SIGNATURE_NOT_FOUND, "{{ function.name }}", "{{ name }}");
}


