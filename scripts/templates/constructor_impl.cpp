static ESValue {{ name|lower }}Function(ESVMInstance* instance)
{
    {% for arg in constructor.arguments %}
    ESValue arg{{ loop.index0 }} = instance->currentExecutionContext()->readArgument({{ loop.index0 }});
        {% if constructor.arguments[loop.index0].type.kind == 'StringType' %}
            {% if constructor.arguments[loop.index0].optional %}
    if (arg{{ loop.index0 }}.isUndefined()) {
        arg{{ loop.index0 }} = ESString::create({{ constructor.arguments[loop.index0].default }});
    }
            {% endif %}
    String* data{{ loop.index0 }} = String::fromUTF8(arg{{ loop.index0 }}.toString()->utf8Data());
        {% elif constructor.arguments[loop.index0].type.kind == 'PrimitiveType' %}
            {% if constructor.arguments[loop.index0].optional %}
    if (arg{{ loop.index0 }}.isUndefined()) {
        arg{{ loop.index0 }} = ESValue({{ constructor.arguments[loop.index0].default }});
    }
            {% endif %}
            {% if constructor.arguments[loop.index0].type.name == 'boolean' %}
    bool data{{ loop.index0 }} = arg{{ loop.index0 }}.asBoolean();
            {% elif constructor.arguments[loop.index0].type.name == 'byte' or
                    constructor.arguments[loop.index0].type.name == 'short' or
                    constructor.arguments[loop.index0].type.name == 'long' %}
    int32_t data{{ loop.index0 }} = arg{{ loop.index0 }}.asInt32();
            {% elif constructor.arguments[loop.index0].type.name == 'octet' or
                    constructor.arguments[loop.index0].type.name == 'unsigned short' or
                    constructor.arguments[loop.index0].type.name == 'unsigned long' %}
    uint32_t data{{ loop.index0 }} = arg{{ loop.index0 }}.asUInt32();
            {% elif constructor.arguments[loop.index0].type.name == 'long long' or
                    constructor.arguments[loop.index0].type.name == 'unsigned long long' or
                    constructor.arguments[loop.index0].type.name == 'float' or
                    constructor.arguments[loop.index0].type.name == 'unrestricted float' or
                    constructor.arguments[loop.index0].type.name == 'double' or
                    constructor.arguments[loop.index0].type.name == 'unrestricted double' %}
    double data{{ loop.index0 }} = arg{{ loop.index0 }}.asDouble();
            {% else %}
    // ERROR: Unexpected primitive type : {{ constructor.arguments[loop.index0].type.name }}
    STARFISH_ASSERT_NOT_REACHED();
            {% endif %}
        {% elif constructor.arguments[loop.index0].type.kind == 'Typeref' %}
    // TODO: implement TypeRef
    STARFISH_ASSERT_NOT_REACHED();
        {% else %}
    // TODO: implement Any, Sequence, UnionType or Promise
    STARFISH_ASSERT_NOT_REACHED();
        {% endif %}
    {% endfor %}
    Window* window = instance->globalObject()->extraPointerData();
    {{ name }}* {{ name|lower }} =
        new {{ name }}(window->document(),
    {% for arg in constructor.arguments %}
        {% if loop.last %}
            data{{ loop.index0 }});
        {% else %}
            data{{ loop.index0 }},
        {% endif %}
    {% endfor %}

    return {{ name|lower }}->scriptValue();
}