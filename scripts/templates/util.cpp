{% macro ifdef(flags) -%}
{% if flags %}
    {% for flag in flags %}
#ifdef {{ flag }}
    {% endfor %}
{{ caller()|trim }}
    {% for flag in flags %}
#endif
    {% endfor %}

  {% else %}
{{ caller() }}
{% endif %}
{%- endmacro %}

{% macro attr_name(attribute) -%}
  {{- attribute.rename|default(attribute.name) -}}
{%- endmacro %}

{% macro gen_primitive_type_str(type) -%}
  {% if type.name == 'boolean' %}
  {{- 'bool' -}}
  {% elif type.name in ['byte', 'short', 'long'] %}
  {{- 'int32_t' -}}
  {% elif type.name in ['octet', 'unsigned short', 'unsigned long'] %}
  {{- 'uint32_t' -}}
  {% elif type.name in ['long long', 'unsigned long long',
                  'float', 'unrestricted float',
                  'double','unrestricted double'] %}
  {{- 'double' -}}
  {% endif %}
{%- endmacro %}

{% macro gen_typeref_type_str(type) -%}
    {% if type.name in enums %}
        {{- type.name -}}
    {% elif type.name in typedefs %}
        {% if typedefs[type.name].kind == 'StringType' %}
            {{- type.name -}}
        {% elif typedefs[type.name].kind == 'PrimitiveType' %}
            {{- type.name -}}
        {% elif typedefs[type.name].kind == 'Typeref' %}
            {{- gen_typeref_type_str(typedefs[type.name]) -}}
        {% endif %}
    {% else %}
        {{- '%s*'|format(type.name) -}}
    {% endif %}
{%- endmacro %}

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
        {{- gen_typeref_type_str(type) -}}
    {% elif type.kind == 'PrimitiveType' %}
        {{- gen_primitive_type_str(type) -}}
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
    {%- elif arg.type.kind == 'StringType' %}
    // NOTE ESNull or ESUndefined to "null" or "undefined"
    {{ gen_from_esvalue(index, arg.type) }}
    {% else %}
    if (arg{{index}}.isUndefinedOrNull()) {
        instance->throwError(ESValue(
                TypeError::create(ESString::create("Wrong argument"))));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else {
        {{ gen_from_esvalue(index, arg.type) }}
    }
    {% endif %}
{% endmacro -%}

{%- macro declare_typeref_return_value(type, use_nullable_struct) -%}
    {% if type.name in enums %}
        {{- '%s result;'|format(gen_type_str(type)) -}}
    {% elif type.name in typedefs %}
        {% if typedefs[type.name].kind == 'StringType' %}
            {{- '%s result;'|format(gen_type_str(type)) -}}
        {% elif typedefs[type.name].kind == 'PrimitiveType' %}
            {{- '%s result;'|format(gen_type_str(type)) -}}
        {% elif typedefs[type.name].kind == 'Typeref' %}
            {{- declare_typeref_return_value(typedefs[type.name], use_nullable_struct) -}}
        {% endif %}
    {% else %}
        {{- '%s result = nullptr;'|format(gen_type_str(type)) -}}
    {% endif %}
{%- endmacro -%}

{%- macro declare_return_value(type, use_nullable_struct) -%}
    {% if use_nullable_struct %}
        {{- 'Nullable<%s> result;'|format(gen_type_str(type)) -}}
    {% elif type.kind == 'Typeref' %}
        {{- declare_typeref_return_value(type, use_nullable_struct) -}}
    {% elif type.name != 'void' %}
        {{- '%s result;'|format(gen_type_str(type)) -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_typeref_return_assert(type) -%}
    {% if type.name in enums %}
    {% elif type.name in typedefs %}
        {% if typedefs[type.name].kind == 'Typeref' %}
            {{- gen_typeref_return_assert(typedefs[type.name]) -}}
        {% endif %}
    {% else %}
        {{- 'STARFISH_ASSERT(result != nullptr);' -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_return_assert(type) -%}
    {% if type.kind == 'Typeref' and not type.nullable -%}
        {{- gen_typeref_return_assert(type) -}}
    {%- endif %}
{%- endmacro -%}

{%- macro gen_typeref_return_code(type, var_name) -%}
    {% if type.name in enums %}
        {{- 'return toJSString(%s);'|format(var_name) -}}
    {% elif type.name in typedefs %}
        {% if typedefs[type.name].kind == 'StringType' %}
            {{- 'return toJSString(%s);'|format(var_name) -}}
        {% elif typedefs[type.name].kind == 'PrimitiveType' %}
            {{- 'return ESValue(%s);'|format(var_name) -}}
        {% elif typedefs[type.name].kind == 'Typeref' %}
            {{- gen_typeref_return_code(typedefs[type.name], var_name) -}}
        {% endif %}
    {% else %}
        {{- '%s->scriptValue();'|format(var_name) -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_return_code(type, var_name) -%}
    {% if type.kind == 'StringType' %}
        {{- 'return toJSString(%s);'|format(var_name) -}}
    {%- elif type.kind == 'Any' %}
        {{- 'return parseJSON(%s);'|format(var_name) -}}
    {%- elif type.kind == 'Typeref' %}
        {{- gen_typeref_return_code(type, var_name) -}}
    {%- elif type.name in ['boolean', 'long', 'short', 'unsigned long', 'unsigned short', 'double', 'long long', 'unsigned long long'] %}
        {{- 'return ESValue(%s);'|format(var_name) -}}
    {%- else %}
        {{- 'STARFISH_RELEASE_ASSERT_NOT_REACHED();' -}}
    {% endif %}
{%- endmacro -%}

{%- macro handle_return(return_type, has_return, use_nullable_struct) -%}
    {% if not has_return -%}
    return ESValue(ESValue::ESUndefined);
    {%- elif use_nullable_struct -%}
    if (!result.hasValue()) {
        return ESValue(ESValue::ESNull);
    } else {
        {{ gen_type_str(return_type) }} result_value = result.getValue();
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