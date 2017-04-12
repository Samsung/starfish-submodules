
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
        {% if arg.type.kind == 'StringType' %}
    {{ gen_type_str(arg.type) }} value{{ index }} = String::fromUTF8({{ arg.default }});
        {% else %}
    {{ gen_type_str(arg.type) }} value{{ index }} = {{ arg.default }};
        {% endif %}
    {% else %}
    {{ gen_type_str(arg.type) }} value{{ index }};
    {% endif %}
    {% if arg.treat_null_as and arg.treat_null_as == 'EmptyString' %}
    if (arg{{index}}.isUndefinedOrNull()) {
        // Null/Undefined argument is treated as EmptyString
        value{{ index }} = String::emptyString;
    } else {
        {{ gen_from_esvalue(index, arg.type) }}
    }
    {% elif arg.optional %}
        {% if not arg.default and not uniformed_call %}
    if (arg{{index}}.isUndefinedOrNull()) {
        validArgCount--;
    } else {
        {{ gen_from_esvalue(index, arg.type) }}
    }
        {% else %}
    if (!arg{{index}}.isUndefinedOrNull()) {
        {{ gen_from_esvalue(index, arg.type) }}
    }
        {% endif %}
    {%- elif arg.type.kind == 'Typeref' %}
    if (arg{{index}}.isUndefinedOrNull()) {
        instance->throwError(ESValue(
                TypeError::create(ESString::create("Wrong argument"))));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else {
        {{ gen_from_esvalue(index, arg.type) }}
    }
    {% else %}
    {{ gen_from_esvalue(index, arg.type) }}
    {% endif %}
{% endmacro -%}

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

{%- macro function_code_normal(class_name, function) -%}
    {% set max_arg = function.arguments|length %}
    {% set min_passing_count = function.min_passing_count|default(0) %}
    {% set min_passed_count = function.min_passed_count|default(0) %}
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
    size_t validArgCount = {{ function.arguments|length }};
    {% endif %}
    {% for arg in function.arguments %}
    ESValue arg{{loop.index - 1}} = instance->currentExecutionContext()->readArgument({{loop.index - 1}});
    {% endfor %}

    {% for arg in function.arguments %}
    {{ handle_arg(loop.index - 1, arg) }}
    {{- 'Error : Wrong argument type' | assert_true(arg.type.name in ['void']) -}}
    {{- 'Error : Unimplemented argument type' | assert_true(arg.type.name in ['object', 'Sequence', 'UnionType', 'Promise']) -}}
    {% endfor %}
    {{ class_name }}* result = nullptr;
    {{ gen_native_call_code(max_arg, min_passing_count, class_name, function, uniformed_call) }}
    return result->scriptValue();
{% endmacro -%}

{% if constructor.custom %}
extern ESValue {{ name|lower }}Constructor(ESVMInstance* instance);
{% else %}
static ESValue {{ name|lower }}Constructor(ESVMInstance* instance)
{
    {{- function_code_normal(name, constructor) -}}
}
{% endif %}

