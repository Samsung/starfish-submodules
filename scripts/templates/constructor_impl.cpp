{# TODO replace 'temp_util_for_function.cpp' to 'util.cpp' #}
{% import 'util.cpp' as util_macro %}
{% if constructor.custom %}
extern ESValue {{ name|lower }}Constructor(ESVMInstance* instance);

{% else %}
static ESValue {{ name|lower }}Constructor(ESVMInstance* instance)
{
    {% set max_arg = constructor.arguments|length %}
    {% set min_passing_count = constructor.min_passing_count|default(0) %}
    {% set min_passed_count = constructor.min_passed_count|default(0) %}
    {% set uniformed_call = (max_arg == min_passing_count) %}
    if (!instance->currentExecutionContext()->isNewExpression()) {
        auto msg = ESString::create("Please use the 'new' operator");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    {% if min_passed_count != 0 %}
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < {{ min_passed_count }}) {
        auto msg = ESString::create("Not enough arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    {% endif %}
    {% if not uniformed_call %}
    size_t validArgCount = {{ constructor.arguments|length }};
    {% endif %}
    {% for arg in constructor.arguments %}
    ESValue arg{{loop.index - 1}} = instance->currentExecutionContext()->readArgument({{loop.index - 1}});
    {% endfor %}
    {% for arg in constructor.arguments -%}
    {{ util_macro.handle_arg(arg, 'arg%d'|format(loop.index - 1), 'value%d'|format(loop.index - 1)) }}
    {% endfor %}
    {{ name }}* result = nullptr;
    {% set call_with = 'callWith' if constructor.call_with else '' %}
    {% set call_with_comma = 'callWith, ' if call_with|length > 0 else '' %}
    {% if constructor.call_with  == 'Document' %}
    Window* window = (Window*)instance->globalObject()->extraPointerData();
    Document* callWith = window->document();
    {% elif constructor.call_with  == 'Starfish' %}
    Window* window = (Window*)instance->globalObject()->extraPointerData();
    StarFish* callWith = window->starFish();
    {% endif %}
    // Call native function (nargs: {{max_arg if uniformed_call else '%s-%s'|format(min_passing_count, max_arg)}})
    {% if max_arg == 0 %}
    result = new {{name}}({{call_with}});
    {% elif uniformed_call %}
    result = new {{name}}({{call_with_comma}}{{ 'value'|to_arg_syntax(0, max_arg) }});
    {% else %}
    if (validArgCount == {{min_passing_count|string}}) {
        {% if min_passing_count == 0 %}
        result = new {{name}}({{call_with}});
        {% else %}
        result = new {{name}}({{call_with_comma}}{{'value'|to_arg_syntax(0, min_passing_count)}});
        {% endif %}
        {% for count in range(min_passing_count + 1, max_arg + 1) %}
    } else if (validArgCount == {{count|string}}) {
        result = new {{name}}({{call_with_comma}}{{'value'|to_arg_syntax(0, count)}});
        {% endfor %}
    }
    {% endif %}
    return result->scriptValue();
}

{% endif %}

