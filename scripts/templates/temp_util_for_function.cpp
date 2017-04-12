{%- macro gen_from_esvalue(type, aname, vname) -%}
    {% if type.kind == 'StringType' %}
        {{- '%s = toBrowserString(%s);'|format(vname, aname) -}}
    {% elif type.kind == 'Any' %}
        {{- '%s = jsonStringify(%s);'|format(vname, aname) -}}
    {% elif type.kind == 'Typeref' %}
        {{- 'CHECK_TYPEOF(%s, %s)'|format(aname, type.name) }}
        {{ '%s = (%s*)(%s.asESPointer()->asESObject()->extraPointerData());'|format(vname, type.name, aname) -}}
    {% elif type.name == 'boolean' %}
        {{- '%s = %s.toBoolean();'|format(vname, aname) -}}
    {% elif type.name in ['long', 'short'] %}
        {{- '%s = %s.toInt32();'|format(vname, aname) -}}
    {% elif type.name in ['unsigned long', 'unsigned short'] %}
        {{- '%s = %s.toUInt32();'|format(vname, aname) -}}
    {% elif type.name in ['double', 'long long', 'unsigned long long'] %}
        {{- '%s = %s.toNumber();'|format(vname, aname) -}}
    {% elif type.kind == 'Dictionary' %}
        {{- '%s = to%sFromESValue(%s);'|format(vname, type.name, aname) -}}
    {% endif %}
{%- endmacro -%}

{%- macro gen_type_str(type) -%}
    {% if type.kind in ['StringType', 'Any'] %}
        {{- 'String*' -}}
    {% elif type.kind == 'Typeref' %}
        {{- '%s*'|format(type.name) -}}
    {% elif type.kind in ['PrimitiveType', 'Dictionary'] %}
        {{- type.name -}}
    {% endif %}
{%- endmacro -%}

{%- macro handle_arg(arg, aname, vname) -%}
    {{ '// Handle argument %s'|format(aname) }}
    {% if arg.default %}
        {% if arg.type.kind == 'StringType' %}
    {{ gen_type_str(arg.type) }} {{ vname }} = String::fromUTF8({{ arg.default }});
        {% else %}
    {{ gen_type_str(arg.type) }} {{ vname }} = {{ arg.default }};
        {% endif %}
    {% else %}
    {{ gen_type_str(arg.type) }} {{ vname }};
    {% endif %}
    {% if arg.treat_null_as and arg.treat_null_as == 'EmptyString' %}
    if ({{ aname }}.isUndefinedOrNull()) {
        // Null/Undefined argument is treated as EmptyString
        {{ vname }} = String::emptyString;
    } else {
        {{ gen_from_esvalue(arg.type, aname, vname) }}
    }
    {% elif arg.optional or arg.default %}
        {% if not arg.default and not uniformed_call %}
    if ({{ aname }}.isUndefinedOrNull()) {
        validArgCount--;
    } else {
        {{ gen_from_esvalue(arg.type, aname, vname) }}
    }
        {% else %}
    if (!{{ aname }}.isUndefinedOrNull()) {
        {{ gen_from_esvalue(arg.type, aname, vname) }}
    }
        {% endif %}
    {%- elif arg.type.kind == 'Typeref' %}
    if ({{ aname }}.isUndefinedOrNull()) {
        instance->throwError(ESValue(
                TypeError::create(ESString::create("Wrong argument"))));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else {
        {{ gen_from_esvalue(arg.type, aname, vname) }}
    }
    {% else %}
    {{ gen_from_esvalue(arg.type, aname, vname) }}
    {% endif %}
{% endmacro -%}