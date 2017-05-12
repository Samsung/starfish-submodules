{% macro ifdef(flags) -%}
{% if flags %}
    {% for flag in flags %}
#ifdef {{ flag }}
    {% endfor %}
{{ caller() -}}
    {% for flag in flags %}
#endif
    {% endfor %}

  {% else %}
{{ caller() }}
{% endif %}
{%- endmacro %}

{% macro gen_attr_name(attribute) -%}
  {{- attribute.rename|default(attribute.name) -}}
{%- endmacro %}

{%- macro gen_getter_function(attribute, name) -%}
    {% if attribute.getter.custom %}
        {{- '%s%sGetterFunction'|format(attribute.name, name) -}}
    {% else %}
        {{- '%sGetterFunction'|format(attribute.name) -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_setter_function(attribute, name) -%}
    {% if attribute.setter.custom %}
        {{- '%s%sSetterFunction'|format(attribute.name, name) -}}
    {% else %}
        {{- '%sSetterFunction'|format(attribute.name) -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_property_owner(primary_global, unforgeable, name) -%}
  {% if (primary_global or unforgeable) %}
    {{- 'scriptObject()' -}}
  {% else %}
    {{- '%sPrototypeObj'|format(name) -}}
  {% endif %}
{%- endmacro -%}

{################## 'HANDLE ARGUMENTS' ##################}
{% macro gen_primitive_type_str(type) -%}
  {% if type.name == 'boolean' %}
  {{- 'bool' -}}
  {% elif type.name in ['byte', 'short', 'long'] %}
  {{- 'int32_t' -}}
  {% elif type.name in ['octet', 'unsigned short', 'unsigned long'] %}
  {{- 'uint32_t' -}}
  {% elif type.name == 'unsigned long long' %}
  {{- 'uint64_t' -}}
  {% elif type.name == 'long long' %}
  {{- 'int64_t' -}}
  {% elif type.name in ['float', 'double'] %}
  {{- 'double' -}}
  {% endif %}
{%- endmacro %}

{%- macro gen_esvalue_to_native(type, aname, fromattr=False) -%}
    {% if type.kind in string_kinds %}
        {{- 'toBrowserString(%s)'|format(aname) -}}
    {% elif type.kind == 'Any' %}
        {{- '%s'|format(aname) -}}
    {% elif type.kind == 'Typeref' %}
        {{- '(%s*)(%s.asESPointer()->asESObject()->extraPointerData())'|format(type.name, aname) -}}
    {% elif type.kind == 'Dictionary' %}
        {{- 'to%sFromESValue(instance, %s)'|format(type.name, aname) -}}
    {% elif type.kind == 'Callback' %}
        {% if type.name == 'EventListener' and fromattr %}
            {{- '%s::to%s(%s, true)'|format(type.name, type.name, aname) -}}
        {% else %}
            {{- '%s::to%s(%s)'|format(type.name, type.name, aname) -}}
        {% endif %}
    {% elif type.kind == 'PrimitiveType' %}
        {% if type.name == 'boolean' %}
            {{- '%s.toBoolean()'|format(aname) -}}
        {% elif type.name in ['long', 'short'] %}
            {{- '%s.toInt32()'|format(aname) -}}
        {% elif type.name in ['unsigned long', 'unsigned short'] %}
            {{- '%s.toUint32()'|format(aname) -}}
        {% elif type.name in ['unsigned long long', 'long long', 'float', 'double'] %}
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

{%- macro gen_check_type(type, aname, skip_type_check=False) -%}
    {% if type.kind == 'Typeref' and not skip_type_check -%}
        CHECK_TYPEOF({{aname}}, {{type.name}});
    {%- endif %}
{% endmacro -%}

{%- macro gen_check_finite_number(arg, names) -%}
    {% if (arg.type.kind == 'PrimitiveType') and
          (arg.type.name == 'double') and
          (not arg.type.unrestricted) %}
if (!std::isfinite({{names.vname}})) {
            {% if names.attrname %}
    COMPOSE_MESSAGE(reason, ARG_TYPE_IS_NONFINITE, "{{names.attrname}}", "{{names.name}}");
    COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, reason);
    THROW_EXCEPTION(msg);
            {% elif names.fname %}
    COMPOSE_MESSAGE(reason, ARG_TYPE_IS_NONFINITE, "{{names.fname}}", "{{names.name}}");
    COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, reason);
    THROW_EXCEPTION(msg);
            {% elif names.kname %}
    // Not implemented for restricted number for dictionary
    STARFISH_ASSERT_NOT_REACHED();
            {% else %}
    COMPOSE_MESSAGE(reason, ARG_TYPE_IS_NONFINITE, "{{names.name}}");
    COMPOSE_MESSAGE(msg, FAILED_TO_CONSTRUCT, reason);
    THROW_EXCEPTION(msg);
            {% endif %}
}
    {% endif %}
{%- endmacro %}

{%- macro handle_arg(arg, names, fromattr=False, skip_type_check=False, need_counting=False) %}
    {% set use_nullable = arg.type.kind in nullable_kinds and arg.type.nullable %}
    {% set type_exp = gen_type_str(arg.type, use_nullable) %}
    {{ '// Handle argument %s'|format(names.aname) }}
    {###### Declaring native variable of an argument ######}
    {% if arg.default %}
        {% if arg.type.kind in string_kinds and arg.default == 'nullptr' %}
        {# THE ARG SHOULD HAVE NULLABLE OPTION #}
    {{ '%s %s;'|format(type_exp, names.vname) }}
        {% elif arg.type.kind in string_kinds %}
            {% if arg.default == '""' %}
    {{ '%s %s = String::emptyString;'|format(type_exp, names.vname) }}
            {% else %}
    {{ '%s %s = String::fromUTF8(%s);'|format(type_exp, names.vname, arg.default) }}
            {% endif %}
        {% else %}
    {{ '%s %s = %s;'|format(type_exp, names.vname, arg.default) }}
        {% endif %}
    {% elif arg.type.kind in string_kinds and not use_nullable %}
    {{ '%s %s = String::emptyString;'|format(type_exp, names.vname) }}
    {% elif arg.type.kind in pointer_kinds %}
    {{ '%s %s = nullptr;'|format(type_exp, names.vname) }}
    {% else %}
    {{ '%s %s;'|format(type_exp, names.vname) }}
    {% endif %}
    {###### Assigning native variable of an argument ######}
    {% set check_type = gen_check_type(arg.type, names.aname, skip_type_check) %}
    {% set assign_exp = '%s = %s;'|format(names.vname, gen_esvalue_to_native(arg.type, names.aname, fromattr)) %}
    {% set check_finite_number = gen_check_finite_number(arg, names) %}
    {% set assign_exp_with_check = '%s\n%s\n%s'|format(check_type, assign_exp, check_finite_number)|trim %}
    {% if (arg.type.kind == 'Callback') and fromattr %}
    {{ assign_exp|indent(4) }}
    {% else %}
        {%- if (arg.treat_null_as == 'EmptyString') %}
        {# '(1) Has-TreatNullAs' #}
    if (!{{ names.aname }}.isNull()) {
        {{ assign_exp_with_check|indent(8) }}
    }
        {%- elif arg.default %}
        {# '(2) Has-DefaultValue' #}
    if (!{{ names.aname }}.isUndefinedOrNull()) {
        {{ assign_exp_with_check|indent(8) }}
    }
        {%- elif arg.optional %}
        {# '(3) Optional + No-DefaultValue' #}
            {% if need_counting %}
    if (argCounting && {{ names.aname }}.isUndefined()) {
        validArgCount--;
    } else {
        argCounting = false;
        {{ assign_exp_with_check|indent(8) }}
    }
            {%- else %}
    if ({{ names.aname }}.isUndefined()) {
        validArgCount--;
    } else {
        {{ assign_exp_with_check|indent(8) }}
    }
            {%- endif %}
        {%- elif arg.type.nullable %}
        {# '(4) Non-optional + Nullable' #}
    if (!{{ names.aname }}.isUndefinedOrNull()) {
        {{ assign_exp_with_check|indent(8) }}
    }
        {%- else %}
        {# '(5) Non-optional + Non-Nullable + RefTypes' #}
        {# '(6) Non-optional + Non-Nullable + Non-RefTypes' #}
    {{ assign_exp_with_check|indent(4) }}
        {% endif %}
    {% endif %}
{% endmacro -%}

{################## 'HANDLE RETURNS' ##################}
{%- macro gen_declare_return_value_impl(type, vname) -%}
    {% set use_nullable = type.kind in nullable_kinds and type.nullable %}
    {% set type_exp = gen_type_str(type, use_nullable) %}
    {% if type.kind in pointer_kinds %}
    {{- '%s %s = nullptr;'|format(type_exp, vname) -}}
    {% elif type.kind in string_kinds %}
    {{- '%s %s = String::emptyString;'|format(type_exp, vname) -}}
    {% elif type.name != 'void' %}
    {{- '%s %s;'|format(type_exp, vname) -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_declare_return_value(type, vname='result') -%}
// Declare native value (empty when type is void)
    {% if type.kind == 'Promise' %}
#ifdef USE_ES6_FEATURE
    {{ gen_declare_return_value_impl(type, vname) }}
        {% if type.data.name != 'void' %}
#else
    {{ gen_declare_return_value_impl(type.data, vname) }}
        {% endif %}
#endif
    {% else %}
    {{ gen_declare_return_value_impl(type, vname) }}
    {% endif %}
{%- endmacro -%}

{%- macro gen_return_assert(type, vname='result') %}
    {% if type.kind in pointer_kinds and not type.nullable %}
STARFISH_ASSERT({{ vname }} != nullptr);
    {% endif %}
{% endmacro -%}

{%- macro gen_native_to_esvalue(type, var_name='result') -%}
    {%- if type.kind in string_kinds %}
toJSString({{var_name}})
    {%- elif type.kind == 'Any' %}
{{var_name}}
    {%- elif type.kind in pointer_kinds %}
{{var_name}}->scriptValue()
    {%- elif type.kind == 'Dictionary' %}
toESValueFrom{{type.name}}(instance, var_name)
    {%- elif type.name in ['boolean', 'long', 'short', 'unsigned long', 'unsigned short', 'double', 'long long', 'unsigned long long'] %}
ESValue({{var_name}})
    {%- endif %}
{%- endmacro -%}

{%- macro gen_return_code(type, vname='result') -%}
return {{ gen_native_to_esvalue(type, vname) }};
{%- endmacro -%}

{%- macro handle_return_impl(return_type, vname='result') -%}
    {% set use_nullable = return_type.kind in nullable_kinds and return_type.nullable %}
    {% if return_type.name == 'void' -%}
    return ESValue(ESValue::ESUndefined);
    {%- elif use_nullable -%}
    if (!{{ vname }}.hasValue()) {
        return ESValue(ESValue::ESNull);
    }
    {{ gen_type_str(return_type, False) }} {{ vname }}_value = {{ vname }}.getValue();
    {{ gen_return_code(return_type, '%s_value'|format(vname)) }}
    {%- elif return_type.nullable -%}
    if ({{ vname }} == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    {{ gen_return_code(return_type, vname) }}
    {%- else -%}
    {{ gen_return_assert(return_type)|trim }}
    {{ gen_return_code(return_type, vname) }}
    {%- endif %}
{%- endmacro -%}

{%- macro handle_return(return_type, vname='result') -%}
// Return ESValue from native value
    {% if return_type.kind == 'Promise' and not return_type.data.kind in pointer_kinds %}
#ifdef USE_ES6_FEATURE
    return {{ vname }}->scriptValue();
#else
    {{ handle_return_impl(return_type.data)|trim }}
#endif
    {% else %}
    {{ handle_return_impl(return_type)|trim }}
    {% endif %}
{%- endmacro -%}