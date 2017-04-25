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

{################## 'HANDLE ARGUMENTS' ##################}
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

{%- macro gen_esvalue_to_native(type, aname) -%}
    {% if type.kind in string_kinds %}
        {{- 'toBrowserString(%s)'|format(aname) -}}
    {% elif type.kind == 'Any' %}
        {{- '%s'|format(aname) -}}
    {% elif type.kind == 'Typeref' %}
        {{- '(%s*)(%s.asESPointer()->asESObject()->extraPointerData())'|format(type.name, aname) -}}
    {% elif type.kind == 'Dictionary' %}
        {{- 'to%sFromESValue(instance, %s)'|format(type.name, aname) -}}
    {% elif type.kind == 'Callback' %}
        {{- 'new %s(%s)'|format(type.name, aname) -}}
    {% elif type.kind == 'PrimitiveType' %}
        {% if type.name == 'boolean' %}
            {{- '%s.toBoolean()'|format(aname) -}}
        {% elif type.name in ['long', 'short'] %}
            {{- '%s.toInt32()'|format(aname) -}}
        {% elif type.name in ['unsigned long', 'unsigned short'] %}
            {{- '%s.toUint32()'|format(aname) -}}
        {% elif type.name in ['double', 'long long', 'unsigned long long'] %}
            {{- '%s.toNumber()'|format(aname) -}}
        {% endif %}
    {% endif %}
{%- endmacro -%}

{%- macro gen_type_str(type, use_nullable) -%}
    {% if type.kind in string_kinds %}
        {% set type_str = 'String*'%}
    {% elif type.kind == 'Any' %}
        {% set type_str = 'ScriptValue'%}
    {% elif type.kind in pointer_kinds %}
        {% set type_str = '%s*'|format(type.name) %}
    {% elif type.kind == 'PrimitiveType' %}
        {% set type_str  = gen_primitive_type_str(type) %}
    {% else %}
        {% set type_str = '%s'|format(type.name) %}
    {% endif %}
    {% if use_nullable %}
        {{- 'Nullable<%s>'|format(type_str) -}}
    {% else %}
        {{- '%s'|format(type_str) -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_check_type(type, aname) -%}
    {% if type.kind == 'Typeref' -%}
        CHECK_TYPEOF({{aname}}, {{type.name}});
    {% endif %}
{% endmacro -%}

{%- macro handle_arg(arg, aname, vname) %}
    {% set use_nullable = arg.type.kind in nullable_kinds and arg.type.nullable %}
    {% set type_exp = gen_type_str(arg.type, use_nullable) %}
    {{ '// Handle argument %s'|format(aname) }}
    {###### Declaring native variable of an argument ######}
    {% if arg.default %}
        {% if arg.type.kind in string_kinds and arg.default == 'nullptr' %}
        {# THE ARG SHOULD HAVE NULLABLE OPTION #}
    {{ '%s %s;'|format(type_exp, vname) }}
        {% elif arg.type.kind in string_kinds %}
    {{ '%s %s = String::fromUTF8(%s);'|format(type_exp, vname, arg.default) }}
        {% else %}
    {{ '%s %s = %s;'|format(type_exp, vname, arg.default) }}
        {% endif %}
    {% elif arg.type.kind in string_kinds and not use_nullable %}
    {{ '%s %s = String::emptyString;'|format(type_exp, vname) }}
    {% elif arg.type.kind in pointer_kinds %}
    {{ '%s %s = nullptr;'|format(type_exp, vname) }}
    {% else %}
    {{ '%s %s;'|format(type_exp, vname) }}
    {% endif %}
    {###### Assigning native variable of an argument ######}
    {% set assign_exp = '%s%s = %s;'|format(gen_check_type(arg.type, aname),
                              vname, gen_esvalue_to_native(arg.type, aname)) %}
    {%- if (arg.treat_null_as == 'EmptyString') or arg.default %}
    {# '(1) Has-TreatNullAs or Has-DefaultValue' #}
    {# '    (NOTE TreatNullAs may not be with optional)' #}
    if (!{{ aname }}.isUndefinedOrNull()) {
        {{ assign_exp|indent(8) }}
    }
    {%- elif arg.optional %}
    {# '(2) Optional + No-DefaultValue' #}
    if ({{ aname }}.isUndefinedOrNull()) {
        validArgCount--;
    } else {
        {{ assign_exp|indent(8) }}
    }
    {%- elif arg.type.nullable %}
    {# '(4) Non-optional + Nullable' #}
    if (!{{ aname }}.isUndefinedOrNull()) {
        {{ assign_exp|indent(8) }}
    }
    {%- else %}
    {# '(5) Non-optional + Non-Nullable + RefTypes' #}
    {# '(6) Non-optional + Non-Nullable + Non-RefTypes' #}
    {{ assign_exp|indent(4) }}
    {% endif %}
{% endmacro -%}

{################## 'HANDLE RETURNS' ##################}
{%- macro gen_declare_return_value_impl(type) -%}
    {% set use_nullable = type.kind in nullable_kinds and type.nullable %}
    {% set type_exp = gen_type_str(type, use_nullable) %}
    {% if type.kind in pointer_kinds %}
    {{- '%s result = nullptr;'|format(type_exp) -}}
    {% elif type.kind in string_kinds %}
    {{- '%s result = String::emptyString;'|format(type_exp) -}}
    {% elif type.name != 'void' %}
    {{- '%s result;'|format(type_exp) -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_declare_return_value(type) -%}
// Declare return value (empty when void)
    {% if type.kind == 'Promise' %}
#ifdef USE_ES6_FEATURE
    {{ gen_declare_return_value_impl(type) }}
        {% if type.data.name != 'void' %}
#else
    {{ gen_declare_return_value_impl(type.data) }}
        {% endif %}
#endif
    {% else %}
    {{ gen_declare_return_value_impl(type) }}
    {% endif %}
{%- endmacro -%}

{%- macro gen_return_assert(type) %}
    {% if type.kind in pointer_kinds and not type.nullable %}
STARFISH_ASSERT(result != nullptr);
    {% endif %}
{% endmacro -%}

{%- macro gen_return_code(type, var_name) -%}
    {%- if type.kind in string_kinds %}
return toJSString({{var_name}});
    {%- elif type.kind == 'Any' %}
return {{var_name}};
    {%- elif type.kind in pointer_kinds %}
return {{var_name}}->scriptValue();
    {%- elif type.kind == 'Dictionary' %}
return toESValueFrom{{type.name}}(instance, var_name);
    {%- elif type.name in ['boolean', 'long', 'short', 'unsigned long', 'unsigned short', 'double', 'long long', 'unsigned long long'] %}
return ESValue({{var_name}});
    {%- else %}
STARFISH_RELEASE_ASSERT_NOT_REACHED();
    {% endif %}
{%- endmacro -%}

{%- macro handle_return_impl(return_type) -%}
    {% set use_nullable = return_type.kind in nullable_kinds and return_type.nullable %}
    {% if return_type.name == 'void' -%}
    return ESValue(ESValue::ESUndefined);
    {%- elif use_nullable -%}
    if (!result.hasValue()) {
        return ESValue(ESValue::ESNull);
    }
    {{ gen_type_str(return_type, False) }} result_value = result.getValue();
    {{ gen_return_code(return_type, 'result_value') }}
    {%- elif return_type.nullable -%}
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    {{ gen_return_code(return_type, 'result') }}
    {%- else -%}
    {{ gen_return_assert(return_type)|trim }}
    {{ gen_return_code(return_type, 'result') }}
    {%- endif %}
{%- endmacro -%}

{%- macro handle_return(return_type) -%}
// Return ESValue from native value
    {% if return_type.kind == 'Promise' and not return_type.data.kind in pointer_kinds %}
#ifdef USE_ES6_FEATURE
    return result->scriptValue();
#else
    {{ handle_return_impl(return_type.data)|trim }}
#endif
    {% else %}
    {{ handle_return_impl(return_type)|trim }}
    {% endif %}
{%- endmacro -%}