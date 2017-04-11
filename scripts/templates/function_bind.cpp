{% import 'util.cpp' as util_macro %}

{% call util_macro.ifdef(function.flag) %}
    {% set fn_name = '%s%s'|format(function.name, name)
                      if function.custom else function.name %}
    ESString* {{ function.name }}String = ESString::create("{{ function.name }}");
    {% if function.static %}
        {% set object %}
            {{ name }}Function
        {% endset %}
    {% else %}
        {% set object %}
            {{ name }}Function->protoType().asESPointer()->asESObject()
        {% endset %}
    {% endif %}
    {{ object|trim }}->defineDataProperty(
        {{ function.name }}String,
        {{ function.writable|default(True)|lower }},
        {{ function.enumerable|default(True)|lower }},
        {{ function.configurable|default(True)|lower }},
    ESFunctionObject::create(nullptr,
                             {{ fn_name }}Function,
                             {{ fn_name }}String,
                             {{ function.arguments|length|default(0) }},
                             false));
{% endcall %}