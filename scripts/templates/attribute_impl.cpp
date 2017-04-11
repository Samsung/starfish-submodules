{% import 'util.cpp' as util_macro %}
{% call util_macro.ifdef(attribute.flags) %}
{% if attribute.getter %}
{% if attribute.custom_getter %}
extern ESValue {{ attribute.name }}{{ name }}GetterFunction(ESVMInstance* instance);
{% else %}
static ESValue {{ attribute.name }}GetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE({{ name }});
    {% if attribute.getter.return.kind == 'StringType' %}
        {% if attribute.getter.return.nullable %}
    Nullable<String*> v = originalObj->{{ attribute.rename|default(attribute.name) }}();
    if (v.hasValue()) {
        return v.getValue()->toJSString(v);
    }
    return return ESValue(ESValue::ESNull);
        {% else %}
    String* v = originalObj->{{ attribute.rename|default(attribute.name) }}();
    return toJSString(v);
        {% endif %}
    {% elif attribute.getter.return.kind == 'PrimitiveType' %}
        {% if attribute.getter.return.name == 'boolean' %}
            {% if attribute.getter.return.nullable %}
    Nullable<bool> v = originalObj->{{ attribute.rename|default(attribute.name) }}();
            {% else %}
    bool v = originalObj->{{ attribute.rename|default(attribute.name) }}();
            {% endif %}
        {% elif attribute.getter.return.name == 'byte' or
                attribute.getter.return.name == 'short' or
                attribute.getter.return.name == 'long' %}
            {% if attribute.getter.return.nullable %}
    Nullable<int32_t> v = originalObj->{{ attribute.rename|default(attribute.name) }}();
            {% else %}
    int32_t v = originalObj->{{ attribute.rename|default(attribute.name) }}();
            {% endif %}
        {% elif attribute.getter.return.name == 'octet' or
                attribute.getter.return.name == 'unsigned short' or
                attribute.getter.return.name == 'unsigned long' %}
            {% if attribute.getter.return.nullable %}
    Nullable<uint32_t> v = originalObj->{{ attribute.rename|default(attribute.name) }}();
            {% else %}
    uint32_t v = originalObj->{{ attribute.rename|default(attribute.name) }}();
            {% endif %}
        {% elif attribute.getter.return.name == 'long long' or
                attribute.getter.return.name == 'unsigned long long' or
                attribute.getter.return.name == 'float' or
                attribute.getter.return.name == 'unrestricted float' or
                attribute.getter.return.name == 'double' or
                attribute.getter.return.name == 'unrestricted double' %}
            {% if attribute.getter.return.nullable %}
    Nullable<double> v = originalObj->{{ attribute.rename|default(attribute.name) }}();
            {% else %}
    double v = originalObj->{{ attribute.rename|default(attribute.name) }}();
            {% endif %}
        {% else %}
    // ERROR: Unexpected primitive type : {{ attribute.getter.return.name }}
    STARFISH_ASSERT_NOT_REACHED();
        {% endif %}
        {% if attribute.getter.return.nullable %}
    if (v.hasValue()) {
        return ESValue(v.getValue());
    }
    return ESValue(ESValue::ESNull);
        {% else %}
    return ESValue(v);
        {% endif %}
    {% elif attribute.getter.return.kind == 'Typeref' %}
        {% if attribute.getter.return.nullable %}
    {{ attribute.getter.return.name }}* v = originalObj->{{ attribute.rename|default(attribute.name) }}();
    if (v != nullptr) {
        return v->scriptValue();
    }
    return ESValue(ESValue::ESNull);
        {% else %}
            {% if attribute.getter.return.name in enums %}
    {{ attribute.getter.return.name }} v = originalObj->{{ attribute.rename|default(attribute.name) }}();
    return toJSString(v);
            {% else %}
    {{ attribute.getter.return.name }}* v = originalObj->{{ attribute.rename|default(attribute.name) }}();
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
    ESValue originalV = instance->currentExecutionContext()->readArgument(0);
    {% if attribute.setter.arguments[0].type.kind == 'StringType' %}
    String* v;
        {% if attribute.setter.arguments[0].treat_null_as == "EmptyString" %}
    if (originalV.isNull()) {
        v = String::emptyString;
    }
        {% endif %}
    v = toBrowserString(originalV);
    {% elif attribute.setter.arguments[0].type.kind == 'PrimitiveType' %}
        {% if attribute.setter.arguments[0].type.name == 'boolean' %}
    bool v = originalV.asBoolean();
        {% elif attribute.setter.arguments[0].type.name == 'byte' or
                attribute.setter.arguments[0].type.name == 'short' or
                attribute.setter.arguments[0].type.name == 'long' %}
    int32_t v = originalV.asInt32();
        {% elif attribute.setter.arguments[0].type.name == 'octet' or
                attribute.setter.arguments[0].type.name == 'unsigned short' or
                attribute.setter.arguments[0].type.name == 'unsigned long' %}
    uint32_t v = originalV.asUInt32();
        {% elif attribute.setter.arguments[0].type.name == 'long long' or
                attribute.setter.arguments[0].type.name == 'unsigned long long' or
                attribute.setter.arguments[0].type.name == 'float' or
                attribute.setter.arguments[0].type.name == 'unrestricted float' or
                attribute.setter.arguments[0].type.name == 'double' or
                attribute.setter.arguments[0].type.name == 'unrestricted double' %}
    double v = originalV.asDouble();
        {% else %}
    // ERROR: Unexpected primitive type : {{ attribute.setter.arguments[0].name }}
    STARFISH_ASSERT_NOT_REACHED();
        {% endif %}
    {% elif attribute.setter.arguments[0].type.kind == 'Typeref' %}
    // TODO: implement TypeRef
    {% else %}
    // TODO: implement Any, Sequence, UnionType or Promise
    STARFISH_ASSERT_NOT_REACHED();
    {% endif %}
    originalObj->set{{ attribute.name|capitalize }}(v);
    return ESValue();
}
{% endif %}
{% endif %}
{% endcall %}