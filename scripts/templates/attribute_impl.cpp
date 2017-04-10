{% import 'util.cpp' as util_macro %}
{% call util_macro.ifdef(attribute.flag) %}
{% if attribute.getter %}
static ESValue {{ attribute.name }}GetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE({{ name }});
    {% if attribute.getter.return.kind == 'StringType' %}
    String* v = originalObj->{{ attribute.name }}();
        {% if attribute.getter.return.nullable %}
    if (v != nullptr) {
        return toJSString(v);
    }
    return ESValue(ESValue::ESNull);
        {% else %}
    return toJSString(v);
        {% endif %}
    {% elif attribute.getter.return.kind == 'PrimitiveType' %}
        {% if attribute.getter.return.name == 'boolean' %}
    bool v = originalObj->{{ attribute.name }}();
        {% elif attribute.getter.return.name == 'byte' or
                attribute.getter.return.name == 'short' or
                attribute.getter.return.name == 'long' %}
    int32_t v = originalObj->{{ attribute.name }}();
        {% elif attribute.getter.return.name == 'octet' or
                attribute.getter.return.name == 'unsigned short' or
                attribute.getter.return.name == 'unsigned long' %}
    uint32_t v = originalObj->{{ attribute.name }}();
        {% elif attribute.getter.return.name == 'long long' or
                attribute.getter.return.name == 'unsigned long long' or
                attribute.getter.return.name == 'float' or
                attribute.getter.return.name == 'unrestricted float' or
                attribute.getter.return.name == 'double' or
                attribute.getter.return.name == 'unrestricted double' %}
    double v = originalObj->{{ attribute.name }}();
        {% else %}
    // ERROR: Unexpected primitive type : {{ attribute.getter.return.name }}
    STARFISH_ASSERT_NOT_REACHED();
        {% endif %}
        {% if attribute.getter.return.nullable %}
    if (v != nullptr) {
        return ESValue(v);
    }
    return ESValue(ESValue::ESNull);
        {% else %}
    return ESValue(v);
        {% endif %}
    {% elif attribute.getter.return.kind == 'Typeref' %}
    {{ attribute.getter.return.name }}* v = originalObj->{{ attribute.name }}();
        {% if attribute.getter.return.nullable %}
    if (v != nullptr) {
        return v->scriptValue();
    }
    return ESValue(ESValue::ESNull);
        {% else %}
    return v->scriptValue();
        {% endif %}
    {% else %}
    // TODO: implement Any, Sequence, UnionType or Promise
    STARFISH_ASSERT_NOT_REACHED();
    {% endif %}
}
{% endif %}
{% if attribute.setter %}

static ESValue {{ attribute.name }}SetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE({{ name }});
    ESValue originalV = instance->currentExecutionContext()->readArgument(0);
    {% if attribute.setter.arguments[0].kind == 'StringType' %}
    String* v = toBrowserString(originalV);
    {% elif attribute.setter.arguments[0].kind == 'PrimitiveType' %}
        {% if attribute.setter.arguments[0].name == 'boolean' %}
    bool v = originalV.asBoolean();
        {% elif attribute.setter.arguments[0].name == 'byte' or
                attribute.setter.arguments[0].name == 'short' or
                attribute.setter.arguments[0].name == 'long' %}
    int32_t v = originalV.asInt32();
        {% elif attribute.setter.arguments[0].name == 'octet' or
                attribute.setter.arguments[0].name == 'unsigned short' or
                attribute.setter.arguments[0].name == 'unsigned long' %}
    uint32_t v = originalV.asUInt32();
        {% elif attribute.setter.arguments[0].name == 'long long' or
                attribute.setter.arguments[0].name == 'unsigned long long' or
                attribute.setter.arguments[0].name == 'float' or
                attribute.setter.arguments[0].name == 'unrestricted float' or
                attribute.setter.arguments[0].name == 'double' or
                attribute.setter.arguments[0].name == 'unrestricted double' %}
    double v = originalV.asDouble();
        {% else %}
    // ERROR: Unexpected primitive type : {{ attribute.setter.arguments[0].name }}
    STARFISH_ASSERT_NOT_REACHED();
        {% endif %}
    {% elif attribute.setter.arguments[0].kind == 'Typeref' %}
    // TODO: implement TypeRef
    {% else %}
    // TODO: implement Any, Sequence, UnionType or Promise
    STARFISH_ASSERT_NOT_REACHED();
    {% endif %}
    originalObj->set{{ attribute.name|capitalize }}(v);
    return ESValue();
}
{% endif %}
{% endcall %}