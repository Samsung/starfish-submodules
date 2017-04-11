{% import 'util.cpp' as util_macro %}
{% call util_macro.ifdef(attribute.flags) %}
{% with %}
{% if attribute.getter %}
{% if attribute.custom_getter %}
extern ESValue {{ attribute.name }}{{ name }}GetterFunction(ESVMInstance* instance);
{% else %}
static ESValue {{ attribute.name }}GetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE({{ name }});
    {% set getter_type = attribute.getter.return %}
    {% if getter_type.kind == 'StringType' %}
        {% if getter_type.nullable %}
    Nullable<String*> v = originalObj->{{ util_macro.attr_name(attribute) }}();
    if (v.hasValue()) {
        return v.getValue()->toJSString(v);
    }
    return return ESValue(ESValue::ESNull);
        {% else %}
    String* v = originalObj->{{ util_macro.attr_name(attribute) }}();
    return toJSString(v);
        {% endif %}
    {% elif getter_type.kind == 'PrimitiveType' %}
        {% if getter_type.name == 'boolean' %}
            {% if getter_type.nullable %}
    Nullable<bool> v = originalObj->{{ util_macro.attr_name(attribute) }}();
            {% else %}
    bool v = originalObj->{{ util_macro.attr_name(attribute) }}();
            {% endif %}
        {% elif getter_type.name in ['byte', 'short', 'long'] %}
            {% if getter_type.nullable %}
    Nullable<int32_t> v = originalObj->{{ util_macro.attr_name(attribute) }}();
            {% else %}
    int32_t v = originalObj->{{ util_macro.attr_name(attribute) }}();
            {% endif %}
        {% elif getter_type.name in ['octet', 'unsigned short',
                                     'unsigned long'] %}
            {% if getter_type.nullable %}
    Nullable<uint32_t> v = originalObj->{{ util_macro.attr_name(attribute) }}();
            {% else %}
    uint32_t v = originalObj->{{ util_macro.attr_name(attribute) }}();
            {% endif %}
        {% elif getter_type.name in ['long long', 'unsigned long long',
                                     'float', 'unrestricted float',
                                     'double','unrestricted double'] %}
            {% if getter_type.nullable %}
    Nullable<double> v = originalObj->{{ util_macro.attr_name(attribute) }}();
            {% else %}
    double v = originalObj->{{ util_macro.attr_name(attribute) }}();
            {% endif %}
        {% else %}
    // ERROR: Unexpected primitive type : {{ getter_type.name }}
    STARFISH_ASSERT_NOT_REACHED();
        {% endif %}
        {% if getter_type.nullable %}
    if (v.hasValue()) {
        return ESValue(v.getValue());
    }
    return ESValue(ESValue::ESNull);
        {% else %}
    return ESValue(v);
        {% endif %}
    {% elif getter_type.kind == 'Typeref' %}
        {% if getter_type.nullable %}
    {{ getter_type.name }}* v = originalObj->{{ util_macro.attr_name(attribute) }}();
    if (v != nullptr) {
        return v->scriptValue();
    }
    return ESValue(ESValue::ESNull);
        {% else %}
            {% if getter_type.name in enums %}
    {{ getter_type.name }} v = originalObj->{{ util_macro.attr_name(attribute) }}();
    return toJSString(v);
            {% else %}
    {{ getter_type.name }}* v = originalObj->{{ util_macro.attr_name(attribute) }}();
    return v->scriptValue();
            {% endif %}
        {% endif %}
    {% else %}
    // TODO: implement Any, Sequence, UnionType or Promise
    STARFISH_ASSERT_NOT_REACHED();
    {% endif %}
}
{% endif %}
{% endif %}
{% if attribute.setter %}

{% if attribute.custom_setter %}
extern ESValue {{ attribute.name }}{{ name }}SetterFunction(ESVMInstance* instance);
{% else %}
static ESValue {{ attribute.name }}SetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE({{ name }});
    {% set setter_type = attribute.setter.arguments[0].type %}
    ESValue originalV = instance->currentExecutionContext()->readArgument(0);
    {% if setter_type.kind == 'StringType' %}
    String* v;
        {% if attribute.setter.arguments[0].treat_null_as == "EmptyString" %}
    if (originalV.isNull()) {
        v = String::emptyString;
    }
        {% endif %}
    v = toBrowserString(originalV);
    {% elif setter_type.kind == 'PrimitiveType' %}
        {% if setter_type.name == 'boolean' %}
    bool v = originalV.asBoolean();
        {% elif setter_type.name in ['byte', 'short', 'long'] %}
    int32_t v = originalV.asInt32();
        {% elif setter_type.name in ['octet', 'unsigned short',
                                     'unsigned long'] %}
    uint32_t v = originalV.asUInt32();
        {% elif setter_type.name in ['long long', 'unsigned long long',
                                     'float', 'unrestricted float',
                                     'double', 'unrestricted double'] %}
    double v = originalV.asDouble();
        {% else %}
    // ERROR: Unexpected primitive type : {{ attribute.setter.arguments[0].name }}
    STARFISH_ASSERT_NOT_REACHED();
        {% endif %}
    {% elif setter_type.kind == 'Typeref' %}
    // TODO: implement TypeRef
    {% else %}
    // TODO: implement Any, Sequence, UnionType or Promise
    STARFISH_ASSERT_NOT_REACHED();
    {% endif %}
    originalObj->set{{ util_macro.attr_name(attribute)|first_word_capitalize }}(v);
    return ESValue();
}
{% endif %}
{% endif %}
{% endwith %}
{% endcall %}