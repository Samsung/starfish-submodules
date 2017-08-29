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

{%- macro gen_check_type(type, aname) -%}
    {% if type.kind == 'Typeref' %}
        _CHECK_TYPEOF({{ aname }}, {{ type.name }})
    {% elif type.kind == 'Sequence' %}
        ({{ aname }}->isObject() && {{ aname }}->asObject()->isArrayObject())
    {% elif type.kind == 'Promise' %}
        ({{ aname }}->isObject() && {{ aname }}->asObject()->isPromiseObject())
    {% elif type.kind == 'Dictionary' %}
        {{ aname }}->isObject()
    {% elif type.kind == 'Callback' %}
        {{ type.name }}::is{{ type.name }}({{ aname }})
    {% elif type.kind in string_type_kinds %}
        {{ aname }}->isString()
    {% elif type.kind == 'PrimitiveType' %}
        {% if type.name == 'object' %}
            {{ aname }}->isObject()
        {% elif type.name == 'boolean' %}
            {{ aname }}->isBoolean()
        {% elif type.name in number_type_names %}
            {{ aname }}->isNumber()
        {% else %}
            TYPE {{ type.name }} IS NOT SUPPORTED
        {% endif %}
    {% elif type.kind == 'UnionType' %}
        is{{ type.name }}(state, {{ aname }})
    {% elif type.kind == 'SpecialType' %}
        {% if type.name == 'ArrayBuffer' %}
            ({{ aname }}->isObject() && {{ aname }}->asObject()->isArrayBufferObject())
        {% elif type.name == 'ArrayBufferView' %}
            ({{ aname }}->isObject() && {{ aname }}->asObject()->isArrayBufferView())
        {% elif type.name == 'Function' %}
            {{ aname }}->isFunction()
        {% else %}
            TYPE {{ type.name }} IS NOT SUPPORTED
        {% endif %}
    {% else %}
        TYPE {{ type.name }} IS NOT SUPPORTED
    {% endif %}
{%- endmacro -%}

{################## 'HANDLE ARGUMENTS' ##################}
{% macro gen_primitive_type_str(type) -%}
  {% if type.name == 'boolean' %}
bool
  {% elif type.name in ['byte', 'short', 'long'] %}
int32_t
  {% elif type.name in ['octet', 'unsigned short', 'unsigned long'] %}
uint32_t
  {% elif type.name == 'unsigned long long' %}
uint64_t
  {% elif type.name == 'long long' %}
int64_t
  {% elif type.name in ['float', 'double'] %}
double
  {% elif type.name == 'object' %}
ScriptObject
  {% endif %}
{%- endmacro %}

{%- macro gen_esvalue_to_native(type, aname, fromattr=False) -%}
    {% if type.kind in string_type_kinds %}
        {{- 'toBrowserString(state, %s)'|format(aname) -}}
    {% elif type.kind == 'Any' %}
        {{- '%s'|format(aname) -}}
    {% elif type.kind == 'Typeref' %}
        {{- '(%s*)(%s->asObject()->extraData())'|format(type.name, aname) -}}
    {% elif type.kind == 'Dictionary' %}
        {{- 'to%sFromValueRef(state, %s)'|format(type.name, aname) -}}
    {% elif type.kind == 'Callback' %}
        {% if type.name == 'EventListener' and fromattr %}
            {{- '%s::to%s(%s, true)'|format(type.name, type.name, aname) -}}
        {% else %}
            {{- '%s::to%s(%s)'|format(type.name, type.name, aname) -}}
        {% endif %}
    {% elif type.kind == 'UnionType' %}
        {{- 'to%sFromValueRef(state, %s)'|format(type.name, aname) -}}
    {% elif type.kind == 'PrimitiveType' %}
        {% if type.name == 'boolean' %}
            {{- '%s->toBoolean(state)'|format(aname) -}}
        {% elif type.name in ['long', 'short'] %}
            {{- '%s->toInt32(state)'|format(aname) -}}
        {% elif type.name in ['unsigned long', 'unsigned short'] %}
            {{- '%s->toUint32(state)'|format(aname) -}}
        {% elif type.name in ['unsigned long long', 'long long', 'float', 'double'] %}
            {{- '%s->toNumber(state)'|format(aname) -}}
        {% elif type.name == 'object' %}
            {{- '%s->toObject(state)'|format(aname) -}}
        {% endif %}
    {% elif type.kind == 'SpecialType' %}
        {% if type.name == 'ArrayBuffer' %}
            {{- '%s->asObject()->asArrayBufferObject()'|format(aname) -}}
        {% elif type.name == 'ArrayBufferView' %}
            {{- '%s->asObject()->asArrayBufferView()'|format(aname) -}}
        {% elif type.name == 'Function' %}
            {{- '%s->asFunction()'|format(aname) -}}
        {% endif %}
    {% endif %}
{%- endmacro -%}

{%- macro get_arrayobject_to_native(seq, aname, vname) -%}
{% set vector_name = '%sInner'|format(vname) if seq.nullable else vname %}
{% if seq.nullable %}
{{ gen_type_str(seq, False) }} {{ vector_name }};
{% endif %}
int {{ aname }}Size = (int){{ aname }}->asObject()->get(state, ValueRef::create(StringRef::fromASCII("length")))->toNumber(state);
for (int i = 0; i < {{ aname }}Size; i++) {
    {% set use_nullable = seq.data.kind in non_nullable_type_kinds and seq.data.nullable %}
    {% set type_exp = gen_type_str(seq.data, use_nullable) %}
    {% if seq.data.kind == 'Sequence' %}
    DOES NOT SUPPORT NESTED SEQUENCE YET !! (PLEASE USE `CUSTOM`)
    {% endif %}
    ValueRef* itemJS = {{ aname }}->asObject()->get(state, ValueRef::create(i));
    {% if seq.data.kind in pointer_type_kinds %}
    {{type_exp}} itemNV = nullptr;
    {% else %}
    {{type_exp}} itemNV;
    {% endif %}
    {% if seq.data.nullable %}
    if (!itemJS->isUndefinedOrNull()) {
        {{ gen_check_type_exception(seq.data, 'itemJS')|trim }}
        itemNV = {{gen_esvalue_to_native(seq.data, 'itemJS')}};
    }
    {% else %}
    {{ gen_check_type_exception(seq.data, 'itemJS')|trim }}
    itemNV = {{gen_esvalue_to_native(seq.data, 'itemJS')}};
    {% endif %}
    {{vector_name}}.push_back(itemNV);
}
{% if seq.nullable %}
{{ vname }} = {{ vector_name }};
{% endif %}
{%- endmacro -%}

{%- macro gen_type_str(type, use_nullable) -%}
    {% if type.kind in string_type_kinds %}
        {% set type_str = 'String*'%}
    {% elif type.kind == 'Any' %}
        {% set type_str = 'ScriptValue'%}
    {% elif type.kind == 'PrimitiveType' %}
        {% set type_str  = gen_primitive_type_str(type)|trim %}
    {% elif type.kind == 'Sequence' %}
        {% set type_str  = 'GCVector<%s>'|format(gen_type_str(type.data, type.data.kind in non_nullable_type_kinds and type.data.nullable)) %}
    {% elif type.kind == 'SpecialType' %}
        {% if type.name == 'ArrayBuffer' %}
            {% set type_str  = 'ScriptArrayBuffer' %}
        {% elif type.name == 'ArrayBufferView' %}
            {% set type_str  = 'ScriptArrayBufferView' %}
        {% elif type.name == 'Function' %}
            {% set type_str  = 'ScriptFunction' %}
        {% endif %}
    {% elif type.kind in pointer_type_kinds %}
        {% set type_str = '%s*'|format(type.name) %}
    {% else %}
        {% set type_str = '%s'|format(type.name) %}
    {% endif %}
    {% if use_nullable %}
        {{- 'Nullable<%s>'|format(type_str) -}}
    {% else %}
        {{- '%s'|format(type_str) -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_check_type_exception(type, aname, skip_type_check=False) -%}
    {% if type.kind == 'Typeref' and not skip_type_check %}
CHECK_TYPEOF({{aname}}, {{type.name}});
    {% elif type.kind in strong_type_kinds and type.kind != 'Dictionary' and not skip_type_check %}
if (!{{ gen_check_type(type, aname)|trim }}) {
    THROW_EXCEPTION(ILLEGAL_INVOKE);
}
    {% endif %}
{%- endmacro -%}

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
    {% set use_nullable = arg.type.kind in non_nullable_type_kinds and arg.type.nullable %}
    {% set type_exp = gen_type_str(arg.type, use_nullable) %}
    {{ '// Handle argument %s'|format(names.aname) }}
    {###### Declaring native variable of an argument ######}
    {% if arg.default %}
        {% if arg.type.kind in string_type_kinds and arg.default == 'nullptr' %}
        {# THE ARG SHOULD HAVE NULLABLE OPTION #}
    {{ '%s %s;'|format(type_exp, names.vname) }}
        {% elif arg.type.kind in string_type_kinds %}
            {% if arg.default == '""' %}
    {{ '%s %s = String::emptyString;'|format(type_exp, names.vname) }}
            {% else %}
    {{ '%s %s = String::fromUTF8(%s);'|format(type_exp, names.vname, arg.default) }}
            {% endif %}
        {% else %}
    {{ '%s %s = %s;'|format(type_exp, names.vname, arg.default) }}
        {% endif %}
    {% elif arg.type.kind in string_type_kinds and not use_nullable %}
    {{ '%s %s = String::emptyString;'|format(type_exp, names.vname) }}
    {% elif arg.type.kind in pointer_type_kinds %}
    {{ '%s %s = nullptr;'|format(type_exp, names.vname) }}
    {% else %}
    {{ '%s %s;'|format(type_exp, names.vname) }}
    {% endif %}
    {###### Assigning native variable of an argument ######}
    {% set check_type = gen_check_type_exception(arg.type, names.aname, skip_type_check)|trim %}
    {% if arg.type.kind == 'Sequence' %}
    {% set assign_exp = get_arrayobject_to_native(arg.type, names.aname, names.vname) %}
    {% else %}
    {% set assign_exp = '%s = %s;'|format(names.vname, gen_esvalue_to_native(arg.type, names.aname, fromattr)) %}
    {% endif %}
    {% set check_finite_number = gen_check_finite_number(arg, names) %}
    {% set assign_exp_with_check = '%s\n%s\n%s'|format(check_type, assign_exp, check_finite_number)|trim %}
    {% if (arg.type.kind == 'Callback') and fromattr %}
    {{ assign_exp|indent(4) }}
    {% else %}
        {%- if (arg.treat_null_as == 'EmptyString') %}
        {# '(1) Has-TreatNullAs' #}
    if (!{{ names.aname }}->isNull()) {
        {{ assign_exp_with_check|indent(8) }}
    }
        {%- elif arg.default %}
        {# '(2) Has-DefaultValue' #}
    if (!{{ names.aname }}->isUndefinedOrNull()) {
        {{ assign_exp_with_check|indent(8) }}
    }
        {%- elif arg.optional %}
        {# '(3) Optional + No-DefaultValue' #}
            {% if need_counting %}
    if (argCounting && {{ names.aname }}->isUndefined()) {
        validArgCount--;
    } else {
        argCounting = false;
        {{ assign_exp_with_check|indent(8) }}
    }
            {%- else %}
    if ({{ names.aname }}->isUndefined()) {
        validArgCount--;
    } else {
        {{ assign_exp_with_check|indent(8) }}
    }
            {%- endif %}
        {%- elif arg.type.nullable %}
        {# '(4) Non-optional + Nullable' #}
    if (!{{ names.aname }}->isUndefinedOrNull()) {
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
{%- macro gen_declare_return_value_impl(type, vname, is_descriptor) -%}
    {% set use_nullable = type.kind in non_nullable_type_kinds and (type.nullable or is_descriptor) %}
    {% set type_exp = gen_type_str(type, use_nullable) %}
    {% if type.kind in pointer_type_kinds %}
    {{- '%s %s = nullptr;'|format(type_exp, vname) -}}
    {% elif type.kind in string_type_kinds %}
    {{- '%s %s = String::emptyString;'|format(type_exp, vname) -}}
    {% elif type.name != 'void' %}
    {{- '%s %s;'|format(type_exp, vname) -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_declare_return_value(type, vname='result', is_descriptor=False) -%}
// Declare native value (empty when type is void)
    {{ gen_declare_return_value_impl(type, vname, is_descriptor) }}
{%- endmacro -%}

{%- macro gen_return_assert(type, vname='result') %}
    {% if type.kind in pointer_type_kinds and not type.nullable %}
STARFISH_ASSERT({{ vname }} != nullptr);
    {% endif %}
{% endmacro -%}

{%- macro gen_native_to_jsvalue(type, var_name='result') -%}
    {%- if type.kind in string_type_kinds %}
ValueRef::create(toJSString({{var_name}}))
    {%- elif type.kind == 'Any' %}
{{var_name}}
    {%- elif type.kind in ['Dictionary', 'UnionType'] %}
toValueRefFrom{{type.name}}(state, {{var_name}})
    {%- elif type.kind in ['PrimitiveType', 'SpecialType'] %}
ValueRef::create({{var_name}})
    {%- elif type.kind in pointer_type_kinds %}
{{var_name}}->scriptValue()
    {%- endif %}
{%- endmacro -%}

{%- macro gen_return_code(type, vname='result') -%}
    {% if type.kind == 'Sequence' %}
ArrayObjectRef* arrayObj = ArrayObjectRef::create(state);
for (unsigned aidx = 0; aidx < {{ vname }}.size(); aidx++) {
    {% if type.data.kind in non_nullable_type_kinds and type.data.nullable %}
    ValueRef* item = {{ vname }}[aidx].hasValue() ? {{ gen_native_to_jsvalue(type.data, '%s[aidx].getValue()'|format(vname)) }} : ValueRef::createNull();
    {% elif type.data.kind in pointer_type_kinds and type.data.nullable %}
    ValueRef* item = {{ vname }}[aidx] != nullptr ? {{ gen_native_to_jsvalue(type.data, '%s[aidx]'|format(vname)) }} : ValueRef::createNull();
    {% else %}
        {% if type.data.kind in pointer_type_kinds %}
    STARFISH_ASSERT({{ vname }}[aidx] != nullptr);
        {% endif %}
    ValueRef* item = {{ gen_native_to_jsvalue(type.data, '%s[aidx]'|format(vname)) }};
    {% endif %}
    arrayObj->set(state, ValueRef::create(aidx), item);
}
return ValueRef::create(arrayObj);
    {% else %}
return {{ gen_native_to_jsvalue(type, vname) }};
    {% endif %}
{%- endmacro -%}

{%- macro handle_return_impl(return_type, vname='result') -%}
    {% set use_nullable = return_type.kind in non_nullable_type_kinds and return_type.nullable %}
    {% if return_type.name == 'void' -%}
    return ValueRef::createUndefined();
    {%- elif use_nullable -%}
    if (!{{ vname }}.hasValue()) {
        return ValueRef::createNull();
    }
    {{ gen_type_str(return_type, False) }} {{ vname }}Value = {{ vname }}.getValue();
    {{ gen_return_code(return_type, '%sValue'|format(vname))|indent(4) }}
    {%- elif return_type.nullable -%}
    if ({{ vname }} == nullptr) {
        return ValueRef::createNull();
    }
    {{ gen_return_code(return_type, vname)|indent(4) }}
    {%- else -%}
    {{ gen_return_assert(return_type)|trim }}
    {{ gen_return_code(return_type, vname)|indent(4) }}
    {%- endif %}
{%- endmacro -%}

{%- macro handle_return(return_type, vname='result') -%}
// Return ValueRef* from native value
    {{ handle_return_impl(return_type)|trim }}
{%- endmacro -%}
