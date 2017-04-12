{# TODO replace 'temp_util_for_function.cpp' to 'util.cpp' #}
{% import 'temp_util_for_function.cpp' as util_macro %}

{%- macro gen_return_type_str(type) -%}
    {%- if type.name != 'void' %}
        {% if type.kind in ['StringType', 'Any'] %}
            {{- 'String*' -}}
        {% elif type.kind == 'Typeref' %}
            {{- '%s*'|format(type.name) -}}
        {% elif type.kind == 'PrimitiveType' %}
            {{- type.name -}}
        {% endif %}
    {% endif -%}
{%- endmacro -%}

{%- macro declare_return_value(type, use_nullable_struct) -%}
    {% if use_nullable_struct %}
        {{- 'Nullable<%s> result;'|format(gen_return_type_str(type)) -}}
    {% elif type.kind == 'Typeref' %}
        {{- '%s result = nullptr;'|format(gen_return_type_str(type)) -}}
    {% elif type.name != 'void' %}
        {{- '%s result;'|format(gen_return_type_str(type)) -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_return_assert(type) -%}
    {% if type.kind == 'Typeref' and not type.nullable -%}
STARFISH_ASSERT(result != nullptr);
    {%- endif %}
{%- endmacro -%}

{%- macro gen_return_code(type, var_name) -%}
    {% if type.kind == 'StringType' %}
return toJSString({{var_name}});
    {%- elif type.kind == 'Any' %}
return parseJSON({{var_name}});
    {%- elif type.kind == 'Typeref' %}
return {{var_name}}->scriptValue();
    {%- elif type.name in ['boolean', 'long', 'short', 'unsigned long', 'unsigned short', 'double', 'long long', 'unsigned long long'] %}
return ESValue({{var_name}});
    {%- else %}
STARFISH_RELEASE_ASSERT_NOT_REACHED();
    {% endif %}
{%- endmacro -%}

{%- macro handle_return(return_type, has_return, use_nullable_struct) -%}
    {% if not has_return -%}
    return ESValue(ESValue::ESUndefined);
    {%- elif use_nullable_struct -%}
    if (!result.hasValue()) {
        return ESValue(ESValue::ESNull);
    } else {
        {{ gen_return_type_str(return_type) }} result_value = result.getValue();
        {{ gen_return_code(return_type, 'result_value') }}
    }
    {%- elif return_type.nullable -%}
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    } else {
        {{ gen_return_code(return_type, 'result') }}
    }
    {%- else -%}
    {{ gen_return_assert(return_type) }}
    {{ gen_return_code(return_type, 'result') }}
    {%- endif %}
{%- endmacro -%}

{%- macro gen_check_getter_code() -%}
    if (instance->currentExecutionContext()->argumentCount() < 1) {
        auto msg = ESString::create(
            "At least 1 argument required, but only 0 present");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
{%- endmacro -%}

{%- macro gen_native_call_code(max_arg, min_passing_count, function, return_left, uniformed_call) -%}
    {% if max_arg == 0 -%}
        {{return_left}}originalObj->{{function.name}}();
    {%- elif uniformed_call -%}
        {{return_left}}originalObj->{{function.name}}({{ 'value'|to_arg_syntax(0, max_arg) }});
    {%- else -%}
        if (validArgCount == {{min_passing_count|string}}) {
            {{return_left}}originalObj->{{function.name}}({{'value'|to_arg_syntax(0, min_passing_count)}});
        {% for count in range(min_passing_count + 1, max_arg + 1) %}
        } else if (validArgCount == {{count|string}}) {
            {{return_left}}originalObj->{{function.name}}({{'value'|to_arg_syntax(0, count)}});
        {% endfor %}
        }
    {%- endif -%}
{%- endmacro -%}

{%- macro gen_tc_coverage_code(class_name, function) -%}
    {%- if function.check_tc_coverage -%}
#ifdef STARFISH_TC_COVERAGE
    STARFISH_LOG_INFO("{{ class_name }}&&&{{ function.name }}\n");
#endif
    {%- endif -%}
{%- endmacro -%}

{%- macro function_code_normal(class_name, function, use_nullable_struct) -%}
    {% set max_arg = function.arguments|length %}
    {% set min_passing_count = function.min_passing_count|default(0) %}
    {% set min_passed_count = function.min_passed_count|default(0) %}
    {% set uniformed_call = (max_arg == min_passing_count) %}
    {% set has_return = (function.return.name != 'void') %}
    {% set return_left = '' if not has_return else 'result = ' %}
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
    size_t validArgCount = {{ function.arguments|length }};
    {% endif %}
    {# TODO Use util_macro #}
    {{ declare_return_value(function.return, use_nullable_struct) }}
    {% for arg in function.arguments %}
    ESValue arg{{loop.index - 1}} = instance->currentExecutionContext()->readArgument({{loop.index - 1}});
    {% endfor %}

    {% for arg in function.arguments %}
    {{ util_macro.handle_arg(arg, 'arg%d'|format(loop.index - 1), 'value%d'|format(loop.index - 1)) }}
    {{- 'Error : Wrong argument type' | assert_true(arg.type.name in ['void']) -}}
    {{- 'Error : Unimplemented argument type' | assert_true(arg.type.name in ['object', 'Sequence', 'UnionType', 'Promise']) -}}
    {% endfor %}
    // Call native function
    {% if function.raises_exception %}
    try {
        {{ gen_native_call_code(max_arg, min_passing_count, function, return_left, uniformed_call) }}
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    {% else %}
    {
        {{ gen_native_call_code(max_arg, min_passing_count, function, return_left, uniformed_call) }}
    }
    {% endif %}
    {{- gen_tc_coverage_code(class_name, function) }}
    {{- handle_return(function.return, has_return, use_nullable_struct) }}
{% endmacro -%}

{%- macro function_code_ellipsis(class_name, function, use_nullable_struct) %}
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    {% if function.raises_exception %}
    try {
        for (int i = 0; i < argCount; i++) {
            ESValue arg0 = instance->currentExecutionContext()->readArgument(i);
            {{ '%s %s'| format(util_macro.gen_type_str(function.arguments[0].type),
                               util_macro.gen_from_esvalue(0, function.arguments[0].type)) }}
            originalObj->{{function.name}}(value0);
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    {% else %}
    for (int i = 0; i < argCount; i++) {
        ESValue arg0 = instance->currentExecutionContext()->readArgument(i);
        {{ '%s %s'| format(util_macro.gen_type_str(function.arguments[0].type),
                           util_macro.gen_from_esvalue(0, function.arguments[0].type)) }}
        originalObj->{{function.name}}(value0);
    }
    {% endif %}
    {{ gen_tc_coverage_code(class_name, function) }}
    return ESValue(ESValue::ESUndefined);
{% endmacro -%}

{%- macro function_code_getter_index(class_name, function, use_nullable_struct) %}
    {% set has_return = (function.return.name != 'void') %}
    {{ gen_check_getter_code() }}
    {# TODO Use util_macro #}
    {{ declare_return_value(function.return, use_nullable_struct) }}
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
{{ gen_tc_coverage_code(class_name, function) }}
    {{ handle_return(function.return, has_return, use_nullable_struct) }}
{% endmacro -%}

{%- macro function_code_getter_name(class_name, function, use_nullable_struct) %}
    {{ gen_check_getter_code() }}
{{ gen_tc_coverage_code(class_name, function) }}
{% endmacro -%}

{%- if not function.name == '_unnamed_' %}
    {% set has_flag = function.flags and function.flags|length > 0 %}
    {% if has_flag %}
#if defined({{function.flags[0]}})
        {%- for idx in range(1, function.flags|length) %}
            {{-  ' && defined(%s)'|format(function.flags[idx]) -}}
        {% endfor %}

    {% endif %}
    {% if function.custom %}
extern ESValue {{ function.name }}{{ name }}Function(ESVMInstance* instance);
    {% else %}
static ESValue {{ function.name }}Function(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    {% set use_nullable_struct = (function.return.kind in ['StringType', 'Any', 'PrimitiveType']) and function.return.nullable %}
    {% if function.arguments|length > 0 and function.arguments[0].ellipsis %}
        {{- function_code_ellipsis(name, function, use_nullable_struct) -}}
    {% elif not function.is_item_getter %}
        {{- function_code_normal(name, function, use_nullable_struct) -}}
    {% elif function.arguments[0].type.kind == 'StringType' %}
        {{- function_code_getter_name(name, function, use_nullable_struct) -}}
    {% elif function.arguments[0].type.name in ['unsigned long', 'unsigned short']  %}
        {{- function_code_getter_index(name, function, use_nullable_struct) -}}
    {% else %}
        SOMETHING WRONG IN BINDING GENERATOR
        PLEASE CHECK IDL AND GENERATOR
    {% endif %}
}
    {% endif %}
    {% if has_flag %}
#else
static ESValue {{ function.name }}Function(ESVMInstance* instance)
{
    auto msg = ESString::create("Starfish does not support it");
    instance->throwError(ESValue(TypeError::create(msg)));
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}
#endif
    {% endif %}

{% endif %}

