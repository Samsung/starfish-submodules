{% if function.unimplemented %}
{% include 'unimpl_function_impl.cpp' ignore missing %}
{% else %}
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
    if (argc < 1) {
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
{{ gen_native_call_impl(return_type, uniformed_call)|indent(spaces, True) }}
    {% if function.raises_exception %}
    } catch (DOMException* e) {
        state->throwException(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    {%- endif %}
{%- endmacro -%}

{%- macro function_code_normal() %}
    {% if name == 'Window' %}
    GENERATE_WINDOW();
    {% elif not function.static %}
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    {% endif %}
    {% set min_passed_count = function.min_passed_count|default(0) %}
    {% set max_arg = function.arguments|length %}
    {% set uniformed_call = (max_arg == function.min_passing_count) %}
    {% set need_counting = (not uniformed_call) and max_arg - function.min_passed_count > 1 %}
    {% set has_return = (function.return.name != 'void') %}
    {% if min_passed_count != 0 and not skip_type_check %}
    size_t argCount = argc;
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
        {% if not function.arguments[loop.index - 1].ellipsis %}
            {% if loop.index - 1 < min_passed_count %}
    ValueRef* arg{{loop.index - 1}} = argv[{{loop.index - 1}}];
            {% else %}
    ValueRef* arg{{loop.index - 1}} = (argc > {{loop.index - 1}}) ? argv[{{loop.index - 1}}] : ValueRef::createUndefined();
            {% endif %}
        {% endif %}
    {% endfor %}
    {% for idx in range(0, max_arg) %}
    {% set ridx = max_arg - idx - 1 -%}
    {% if function.arguments[ridx].ellipsis %}
    {{ handle_ellipsis(ridx)|trim }}
    {% else %}
    {% set names = {'name': name, 'fname': function.name, 'aname': 'arg%d'|format(ridx), 'vname': 'value%d'|format(ridx)} %}
    {{ util_macro.handle_arg(function.arguments[ridx], names,
                             skip_type_check=skip_type_check,
                             need_counting=need_counting)|trim }}
    {% endif %}
    {% endfor %}
    {% if function.call_with  == 'Document' %}
    Document* callWith = fetchDocument(state->context());
    {% elif function.call_with  == 'Starfish' %}
    StarFish* callWith = fetchStarFish(state->context());
    {% endif %}
    {{ gen_native_call(function.return, uniformed_call) }}
    {{ util_macro.handle_return(function.return)|trim }}
{% endmacro -%}

{%- macro handle_ellipsis(start_idx) %}
    {% set ellp_type = function.arguments[start_idx].type -%}
    {% set type_exp = util_macro.gen_type_str(ellp_type, ellp_type.nullable and ellp_type in non_nullable_type_kinds) -%}
    // Handle ellipsis arguments from index{{start_idx}}
    GCVector<{{type_exp}}> value{{start_idx}};
    {% if function.min_passed_count == 0 or skip_type_check %}
    size_t argCount = argc;
    {% endif %}
    for (size_t i = {{start_idx}}; i < argCount; i++) {
        ValueRef* item = argv[i];
        value{{start_idx}}.push_back({{util_macro.gen_esvalue_to_native(ellp_type, 'item')}});
    }
{% endmacro -%}

{%- macro function_code_getter_index() %}
    {% if name == 'Window' %}
    GENERATE_WINDOW();
    {% elif not function.static %}
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    {% endif %}
    {% set has_return = (function.return.name != 'void') %}
    {% set property_owner = 'window' if name == 'Window' else 'originalObj' %}
    // Class item getter by index
    {{ gen_check_getter_code() }}
    {{ util_macro.gen_declare_return_value(function.return)|trim }}
    ValueRef* arg0 = argv[0];
    ValueRef::ValueIndex idx = arg0->toArrayIndex(state);
    if (idx == ValueRef::InvalidArrayIndexValue) {
        double __number = arg0->toNumber(state);
        if (__number < 0) {
            return scriptNull();
        }
        idx = std::isnan(__number) ? 0 : (uint32_t)__number;
    }
    result = {{ property_owner }}->{{function.name}}(idx);
    {{ util_macro.handle_return(function.return)|trim }}
{% endmacro -%}

{%- macro function_code_stringifier() %}
    {% if function.forward %}
    return {{ gen_function_name(function.forward) }}(state, thisValue, argc, argv, isNewExpression);
    {% else %}
        {{- function_code_normal() -}}
    {% endif %}
{% endmacro -%}

{%- if not function.name == '_unnamed_' %}
    {% set fnname = '%s%s'|format(function.name, function.id) if function.id else function.name %}
    {% set has_flag = function.flags and function.flags|length > 0 %}
    {% call util_macro.ifdef(function.flags) %}
    {% if function.custom %}
extern ValueRef* {{ gen_function_name(function) }}(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression);
    {% else %}
static ValueRef* {{ gen_function_name(function) }}(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    {% if function.is_item_getter and
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
{% endif %}

