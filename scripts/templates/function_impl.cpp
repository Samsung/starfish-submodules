{% import 'util.cpp' as util_macro %}
{%- macro gen_function_name(obj) %}
    {% if obj.kind == 'Attribute' %}
        {{- util_macro.gen_getter_function(obj, name) -}}
    {% else %}
        {% set fnname = '%s%s'|format(obj.name, obj.id) if obj.id else obj.name %}
        {% if obj.custom %}
            {{- '%s%sFunction'|format(fnname, name) -}}
        {% else %}
            {{- '%sFunction'|format(fnname) -}}
        {% endif %}
    {% endif %}
{% endmacro -%}

{%- macro gen_check_getter_code() -%}
    if (instance->currentExecutionContext()->argumentCount() < 1) {
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", "0");
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "{{ function.name }}", "{{ name }}", reason);
        THROW_EXCEPTION(msg);
    }
{%- endmacro -%}

{%- macro gen_native_call_impl(return_type, uniformed_call) -%}
    {% set call_with = 'callWith' if function.call_with else '' %}
    {% set call_with_comma = 'callWith, ' if call_with|length > 0 else '' %}
    {% set return_left = '' if return_type.name == 'void' else 'result = ' %}
    {% set property_owner = 'window' if name == 'Window' else 'originalObj' %}
    {% set calling = '%s::'|format(name) if function.static else '%s->'|format(property_owner) %}
    {% set fnname = function.name if function.rename|length == 0 else function.rename %}
    {% set max_arg = function.arguments|length %}
    {% if max_arg == 0 -%}
{{return_left}}{{calling}}{{fnname}}({{call_with}});
    {%- elif uniformed_call -%}
{{return_left}}{{calling}}{{fnname}}({{call_with_comma}}{{ 'value'|to_arg_syntax(0, max_arg) }});
    {%- else -%}
if (validArgCount == {{function.min_passing_count|string}}) {
    {{return_left}}{{calling}}{{fnname}}({{call_with_comma}}{{'value'|to_arg_syntax(0, function.min_passing_count)}});
        {% for count in range(function.min_passing_count + 1, max_arg + 1) %}
} else if (validArgCount == {{count|string}}) {
    {{return_left}}{{calling}}{{fnname}}({{call_with_comma}}{{'value'|to_arg_syntax(0, count)}});
        {% endfor %}
}
    {%- endif -%}
{%- endmacro -%}

{%- macro gen_native_call(return_type, uniformed_call) -%}
// Call native function (nargs: {{'%s%s'|format('' if uniformed_call else '%s-'|format(function.min_passing_count), function.arguments|length)}})
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

{%- macro function_code_normal() %}
    {% if name == 'Window' %}
    GENERATE_WINDOW();
    {% else %}
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    {% endif %}
    {% set min_passed_count = function.min_passed_count|default(0) %}
    {% set max_arg = function.arguments|length %}
    {% set uniformed_call = (max_arg == function.min_passing_count) %}
    {% set need_counting = (not uniformed_call) and max_arg - function.min_passed_count > 1 %}
    {% set has_return = (function.return.name != 'void') %}
    {% if min_passed_count != 0 and not skip_type_check %}
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < {{ min_passed_count }}) {
        {% set siz = min_passed_count|digit + 1 %}
        char buffer[{{ siz }}];
        snprintf(buffer, {{ siz }}, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "{{ min_passed_count }}", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "{{ fnname }}", "{{ name }}", reason);
        THROW_EXCEPTION(msg);
    }
    {% endif %}
    {% if not uniformed_call %}
    size_t validArgCount = {{ max_arg }};
    {% endif %}
    {% if need_counting %}
    bool argCounting = true;
    {% endif %}
    {{ util_macro.gen_declare_return_value(function.return)|trim }}
    {% for arg in function.arguments %}
    ESValue arg{{loop.index - 1}} = instance->currentExecutionContext()->readArgument({{loop.index - 1}});
    {% endfor %}
    {% for idx in range(0, max_arg) %}
    {% set ridx = max_arg - idx - 1 -%}
    {% set names = {'name': name, 'fname': function.name, 'aname': 'arg%d'|format(ridx), 'vname': 'value%d'|format(ridx)} %}
    {{ util_macro.handle_arg(function.arguments[ridx], names,
                             skip_type_check=skip_type_check,
                             need_counting=need_counting)|trim }}
    {% endfor %}
    {% if function.call_with  == 'Document' %}
    Document* callWith = fetchDocument(instance);
    {% elif function.call_with  == 'Starfish' %}
    StarFish* callWith = fetchStarFish(instance);
    {% endif %}
    {{ gen_native_call(function.return, uniformed_call) }}
    {{ util_macro.handle_return(function.return)|trim }}
{% endmacro -%}

{%- macro function_code_ellipsis() %}
    {% if name == 'Window' %}
    GENERATE_WINDOW();
    {% else %}
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    {% endif %}
    {% set use_nullable = (function.return.kind in nullable_kinds) and function.return.nullable %}
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    {% set type_exp = util_macro.gen_type_str(function.arguments[0].type, use_nullable) %}
    {% set assign_exp = util_macro.gen_esvalue_to_native(function.arguments[0].type, 'arg', False) %}
    {% set property_owner = 'window' if name == 'Window' else 'originalObj' %}
    {% if function.raises_exception %}
    try {
        for (size_t i = 0; i < argCount; i++) {
            ESValue arg = instance->currentExecutionContext()->readArgument(i);
            {{ '%s value = %s;'|format(type_exp, assign_exp) }}
            {{ property_owner }}->{{function.name}}(value);
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    {% else %}
    for (size_t i = 0; i < argCount; i++) {
        ESValue arg = instance->currentExecutionContext()->readArgument(i);
        {{ '%s value = %s;'|format(type_exp, assign_exp) }}
        {{ property_owner }}->{{function.name}}(value);
    }
    {% endif %}
    return ESValue(ESValue::ESUndefined);
{% endmacro -%}

{%- macro function_code_getter_index() %}
    {% if name == 'Window' %}
    GENERATE_WINDOW();
    {% else %}
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    {% endif %}
    {% set has_return = (function.return.name != 'void') %}
    {% set property_owner = 'window' if name == 'Window' else 'originalObj' %}
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
    result = {{ property_owner }}->{{function.name}}(idx);
    {{ util_macro.handle_return(function.return)|trim }}
{% endmacro -%}

{%- macro function_code_stringifier() %}
    {% if function.forward %}
    return {{ gen_function_name(function.forward) }}(instance);
    {% else %}
        {{- function_code_normal() -}}
    {% endif %}
{% endmacro -%}

{%- if not function.name == '_unnamed_' %}
    {% set fnname = '%s%s'|format(function.name, function.id) if function.id else function.name %}
    {% set has_flag = function.flags and function.flags|length > 0 %}
    {% call util_macro.ifdef(function.flags) %}
    {% if function.custom %}
extern ESValue {{ gen_function_name(function) }}(ESVMInstance* instance);
    {% else %}
static ESValue {{ gen_function_name(function) }}(ESVMInstance* instance)
{
    {% if function.arguments|length > 0 and function.arguments[0].ellipsis %}
    {{- function_code_ellipsis() -}}
    {% elif function.is_item_getter and
        function.arguments[0].type.name in ['unsigned long', 'unsigned short']  %}
    {{- function_code_getter_index() -}}
    {% elif function.kind == 'Stringifier' %}
    {{- function_code_stringifier() -}}
    {% else %}
    {{- function_code_normal() -}}
    {% endif %}
}
    {% endif %}
    {% endcall %}
{% endif %}

