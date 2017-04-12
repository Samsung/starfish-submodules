{# TODO replace 'temp_util_for_function.cpp' to 'util.cpp' #}
{% import 'temp_util_for_function.cpp' as util_macro %}

{%- macro gen_native_call_code(max_arg, min_passing_count, class_name, function, uniformed_call) -%}
    {% if max_arg == 0 -%}
    result = new {{ class_name }}(Window->document());
    {%- elif uniformed_call -%}
    result = new {{ class_name }}(Window->document(), {{ 'value'|to_arg_syntax(0, max_arg) }});
    {%- else -%}
    if (validArgCount == {{min_passing_count|string}}) {
        {% if min_passing_count == 0 %}
        result = new {{ class_name }}(Window->document());
        {% else %}
        result = new {{ class_name }}(Window->document(), {{'value'|to_arg_syntax(0, min_passing_count)}});
        {% endif %}
        {% for count in range(min_passing_count + 1, max_arg + 1) %}
    } else if (validArgCount == {{count|string}}) {
        result = new {{ class_name }}(Window->document(), {{'value'|to_arg_syntax(0, count)}});
        {% endfor %}
    }
    {%- endif %}
{%- endmacro -%}

{% if constructor.custom %}
extern ESValue {{ name|lower }}Constructor(ESVMInstance* instance);
{% else %}
static ESValue {{ name|lower }}Constructor(ESVMInstance* instance)
{
    {% set max_arg = constructor.arguments|length %}
    {% set min_passing_count = constructor.min_passing_count|default(0) %}
    {% set min_passed_count = constructor.min_passed_count|default(0) %}
    {% set uniformed_call = (max_arg == min_passing_count) %}
    {% if min_passed_count != 0 %}
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < {{ min_passed_count }}) {
        auto msg = ESString::create("Not enough arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    {% endif %}
    // This function has {{'' if uniformed_call else 'not '}}uniformed function call
    {% if not uniformed_call %}
    size_t validArgCount = {{ constructor.arguments|length }};
    {% endif %}
    {% for arg in constructor.arguments %}
    ESValue arg{{loop.index - 1}} = instance->currentExecutionContext()->readArgument({{loop.index - 1}});
    {% endfor %}

    {% for arg in constructor.arguments %}
    {{ util_macro.handle_arg(arg, 'arg%d'|format(loop.index - 1), 'value%d'|format(loop.index - 1)) }}
    {{- 'Error : Wrong argument type' | assert_true(arg.type.name in ['void']) -}}
    {{- 'Error : Unimplemented argument type' | assert_true(arg.type.name in ['object', 'Sequence', 'UnionType', 'Promise']) -}}
    {% endfor %}
    {{ name }}* result = nullptr;
    {{ gen_native_call_code(max_arg, min_passing_count, name, constructor, uniformed_call) }}
    return result->scriptValue();
}
{% endif %}

