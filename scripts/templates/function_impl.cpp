
{%- macro gen_from_esvalue(index, type) -%}
    {% if type.kind == 'StringType' %}
        {{- 'value%d = toBrowserString(arg%d);'|format(index, index) -}}
    {% elif type.kind == 'Any' %}
        {{- 'value%d = jsonStringify(arg%d);'|format(index, index) -}}
    {% elif type.kind == 'Typeref' %}
        {{- 'CHECK_TYPEOF(arg%d, %s)'|format(index, type.name) }}
        {{- 'value%d = (%s*)(arg%s.asESPointer()->asESObject()->extraPointerData());'|format(index, type.name, index) -}}
    {% elif type.name == 'boolean' %}
        {{- 'value%d = arg%d.toBoolean();'|format(index, index) -}}
    {% elif type.name in ['long', 'short'] %}
        {{- 'value%d = arg%d.toInt32();'|format(index, index) -}}
    {% elif type.name in ['unsigned long', 'unsigned short'] %}
        {{- 'value%d = arg%d.toUInt32();'|format(index, index) -}}
    {% elif type.name in ['double', 'long long', 'unsigned long long'] %}
        {{- 'value%d = arg%d.toNumber();'|format(index, index) -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_type_str(type) -%}
    {% if type.kind in ['StringType', 'Any'] %}
        {{- 'String*' -}}
    {% elif type.kind == 'Typeref' %}
        {{- '%s*'|format(type.name) -}}
    {% elif type.kind == 'PrimitiveType' %}
        {{- type.name -}}
    {% endif %}
{%- endmacro -%}

{%- macro handle_arg(index, arg) -%}
    {{ '// Handle argument[%d]'|format(index) }}
    {% if arg.default %}
    {{ gen_type_str(arg.type) }} value{{ index }} = {{ arg.default }};
    {% else %}
    {{ gen_type_str(arg.type) }} value{{ index }};
    {% endif %}
    if (arg{{index}}.isUndefinedOrNull()) {
        {% if arg.optional %}
            {% if not arg.default and not uniformed_call%}
        validArgCount--;
            {% endif %}
        {% else %}
        instance->throwError(ESValue(
                TypeError::create(ESString::create("Wrong argument"))));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        {% endif %}
    } else {
        {{ gen_from_esvalue(index, arg.type) }}
        {# Need to clamp #}
    }
{% endmacro -%}

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
    {% else%}
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
    if (result.isNull()) {
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
    {% endif %}
{%- endmacro -%}

{%- macro gen_check_getter_code() -%}
    if (instance->currentExecutionContext()->argumentCount() < 1) {
        auto msg = ESString::create(
            "At least 1 argument required, but only 0 present");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
{%- endmacro -%}

{%- macro function_code_normal(function, use_nullable_struct) -%}
    {% set max_arg = function.arguments|length %}
    {% set min_arg = function.min_arg_count|default(0) %}
    {% set uniformed_call = (max_arg == min_arg) %}
    {% set has_return = (function.return.name != 'void') %}
    {% set return_left = '' if not has_return else 'result = ' %}

    // This function has {{'' if uniformed_call else 'not '}}uniformed function call
    {% if not uniformed_call %}
    size_t validArgCount = {{ function.arguments|length }};
    {% endif %}
    {{ declare_return_value(function.return, use_nullable_struct) }}
    {% for arg in function.arguments %}
    ESValue arg{{loop.index - 1}} = instance->currentExecutionContext()->readArgument({{loop.index - 1}});
    {% endfor %}

    {% for arg in function.arguments %}
    {{ handle_arg(loop.index - 1, arg) }}
    {{- 'Error : Wrong argument type' | assert_true(arg.type.name in ['void']) -}}
    {{- 'Error : Unimplemented argument type' | assert_true(arg.type.name in ['object', 'Sequence', 'UnionType', 'Promise']) -}}
    {% endfor %}
    // TODO Catch DOM exceptions
    {% if max_arg == 0 %}
    {{return_left}}originalObj->{{function.name}}();
    {% elif uniformed_call%}
    {{return_left}}originalObj->{{function.name}}({{ 'arg'|to_arg_syntax(0, max_arg) }});
    {% else %}
    if (validArgCount == {{min_arg|string}}) {
        {{return_left}}originalObj->{{function.name}}({{'arg'|to_arg_syntax(0, min_arg)}});
    {% for count in range(min_arg + 1, max_arg + 1) %}
    } else if (validArgCount == {{count|string}}) {
        {{return_left}}originalObj->{{function.name}}({{'arg'|to_arg_syntax(0, count)}});
    {% endfor %}
    }
    {% endif %}
    {{ handle_return(function.return, has_return, use_nullable_struct) }}
{%- endmacro -%}

{%- macro function_code_getter_index(function, use_nullable_struct) %}
    {% set has_return = (function.return.name != 'void') %}
    {{ gen_check_getter_code() }}
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
    {{ handle_return(function.return, has_return, use_nullable_struct) }}
{% endmacro -%}

{% macro function_code_getter_name(function, use_nullable_struct) %}
    {{ gen_check_getter_code() }}
{% endmacro -%}

{%- if not function.name == '_unnamed_' %}
static ESValue {{ function.name }}Function(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    {% set use_nullable_struct = (function.return.kind in ['StringType', 'Any', 'PrimitiveType']) and function.return.nullable %}
    {% if not function.is_item_getter %}
        {{- function_code_normal(function, use_nullable_struct) -}}
    {% elif function.arguments[0].type.kind == 'StringType' %}
        {{- function_code_getter_name(function, use_nullable_struct) -}}
    {% elif function.arguments[0].type.name in ['unsigned long', 'unsigned short']  %}
        {{- function_code_getter_index(function, use_nullable_struct) -}}
    {% else %}
        SOMETHING WRONG IN BINDING GENERATOR
        PLEASE CHECK IDL AND GENERATOR
    {% endif %}
}

{% endif %}

