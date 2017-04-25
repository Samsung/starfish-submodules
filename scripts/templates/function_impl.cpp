{% import 'util.cpp' as util_macro %}

{%- macro gen_check_getter_code() -%}
    if (instance->currentExecutionContext()->argumentCount() < 1) {
        auto msg = ESString::create(
            "At least 1 argument required, but only 0 present");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
{%- endmacro -%}

{%- macro gen_native_call_impl(return_type, uniformed_call) -%}
    {% set return_left = '' if return_type.name == 'void' else 'result = ' %}
    {% set calling = '%s::'|format(name) if function.static else 'originalObj->' %}
    {% set max_arg = function.arguments|length %}
    {% if max_arg == 0 -%}
{{return_left}}{{calling}}{{function.name}}();
    {%- elif uniformed_call -%}
{{return_left}}{{calling}}{{function.name}}({{ 'value'|to_arg_syntax(0, max_arg) }});
    {%- else -%}
if (validArgCount == {{function.min_passing_count|string}}) {
    {{return_left}}{{calling}}{{function.name}}({{'value'|to_arg_syntax(0, function.min_passing_count)}});
        {% for count in range(function.min_passing_count + 1, max_arg + 1) %}
} else if (validArgCount == {{count|string}}) {
    {{return_left}}{{calling}}{{function.name}}({{'value'|to_arg_syntax(0, count)}});
        {% endfor %}
}
    {%- endif -%}
{%- endmacro -%}

{%- macro gen_native_call(return_type, uniformed_call) -%}
// Call native function (nargs: {{function.arguments|length if uniformed_call else '%s-%s'|format(min_passing_count, max_arg)}})
    {% set spaces = 8 if function.raises_exception else 4 %}
    {% if function.raises_exception %}
    try {
    {% endif %}
    {% if return_type.kind == 'Promise' and not return_type.data.kind in pointer_kinds %}
#ifdef USE_ES6_FEATURE
{{ gen_native_call_impl(return_type, uniformed_call)|indent(spaces, True) }}
#else
{{ gen_native_call_impl(return_type.data, uniformed_call)|indent(spaces, True) }}
#endif
    {% else %}
{{ gen_native_call_impl(return_type, uniformed_call)|indent(spaces, True) }}
    {% endif %}
    {% if function.raises_exception %}
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    {%- endif %}
{%- endmacro -%}

{%- macro function_code_normal() -%}
    {% set min_passed_count = function.min_passed_count|default(0) %}
    {% set uniformed_call = (function.arguments|length == function.min_passing_count) %}
    {% set has_return = (function.return.name != 'void') %}
    {% if min_passed_count != 0 %}
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < {{ min_passed_count }}) {
        {% set siz = min_passed_count|digit %}
        char buffer[{{ siz }} + 1];
        snprintf(buffer, {{ siz }}, "%zd", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                            "{{ fnname }}", "{{ name }}", "{{ min_passed_count }}", buffer);
    }
    {% endif %}
    {% if not uniformed_call %}
    size_t validArgCount = {{ function.arguments|length }};
    {% endif %}
    {{ util_macro.gen_declare_return_value(function.return)|trim }}
    {% for arg in function.arguments %}
    ESValue arg{{loop.index - 1}} = instance->currentExecutionContext()->readArgument({{loop.index - 1}});
    {% endfor %}
    {% for arg in function.arguments -%}
    {{ util_macro.handle_arg(arg, 'arg%d'|format(loop.index - 1), 'value%d'|format(loop.index - 1)) }}
    {% endfor %}
    {{ gen_native_call(function.return, uniformed_call) }}
    {{ util_macro.handle_return(function.return)|trim }}
{% endmacro -%}

{%- macro function_code_ellipsis() %}
    {% set use_nullable = (function.return.kind in nullable_kinds) and function.return.nullable %}
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    {% set type_exp = util_macro.gen_type_str(function.arguments[0].type, use_nullable) %}
    {% set assign_exp = util_macro.gen_esvalue_to_native(function.arguments[0].type, 'arg') %}
    {% if function.raises_exception %}
    try {
        for (size_t i = 0; i < argCount; i++) {
            ESValue arg = instance->currentExecutionContext()->readArgument(i);
            {{ '%s value = %s;'|format(type_exp, assign_exp) }}
            originalObj->{{function.name}}(value);
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    {% else %}
    for (size_t i = 0; i < argCount; i++) {
        ESValue arg = instance->currentExecutionContext()->readArgument(i);
        {{ '%s value = %s;'|format(type_exp, assign_exp) }}
        originalObj->{{function.name}}(value);
    }
    {% endif %}
    return ESValue(ESValue::ESUndefined);
{% endmacro -%}

{%- macro function_code_getter_index() %}
    {% set has_return = (function.return.name != 'void') %}
    // Class item getter by index
    {{ gen_check_getter_code() }}
    {{ util_macro.gen_declare_return_value(function.return)|trim }}
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    uint32_t idx = arg0.toIndex();
    if (idx == ESValue::ESInvalidIndexValue) {
        double __number = arg0.toNumber();
        if (__number < 0) {
            return ESValue(ESValue::ESNull);
        }
        idx = std::isnan(__number) ? 0 : (uint32_t)__number;
    }
    result = originalObj->{{function.name}}(idx);
    {{ util_macro.handle_return(function.return)|trim }}
{% endmacro -%}

{%- if not function.name == '_unnamed_' %}
    {% set fnname = '%s%s'|format(function.name, function.id) if function.id else function.name %}
    {% set has_flag = function.flags and function.flags|length > 0 %}
    {% if has_flag %}
#if defined({{function.flags[0]}})
        {%- for idx in range(1, function.flags|length) %}
            {{-  ' && defined(%s)'|format(function.flags[idx]) -}}
        {% endfor %}

    {% endif %}
    {% if function.custom %}
extern ESValue {{ fnname }}{{ name }}Function(ESVMInstance* instance);
    {% else %}
static ESValue {{ fnname }}Function(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    {% if function.arguments|length > 0 and function.arguments[0].ellipsis %}
        {{- function_code_ellipsis() -}}
    {% elif function.is_item_getter and
        function.arguments[0].type.name in ['unsigned long', 'unsigned short']  %}
        {{- function_code_getter_index() -}}
    {% else %}
        {{- function_code_normal() -}}
    {% endif %}
}
    {% endif %}
    {% if has_flag %}
#else
static ESValue {{ fnname }}Function(ESVMInstance* instance)
{
    auto msg = ESString::create("Starfish does not support it");
    instance->throwError(ESValue(TypeError::create(msg)));
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}
#endif
    {% endif %}

{% endif %}

