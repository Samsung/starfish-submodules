{% import 'util.cpp' as util_macro %}
{% macro bind_const(object, attribute) -%}
{{ object }}
        ->defineDataProperty({{ attribute.name }}String,
                              {{ "false" if attribute.const else attribute.writable|default(True)|lower }},
                              {{ enumerable|default(True)|lower() }},
                              {{ "false" if attribute.const else attribute.configurable|default(True)|lower }},
                              {{ attribute.name }}Value);
{%- endmacro %}
{% call util_macro.ifdef(attribute.flag) %}
{% with %}
    ESString* {{ attribute.name }}String = ESString::create("{{ attribute.name }}");
    {% if attribute.const %}
        {% set prototype_object %}
    {{ name }}Function->protoType()
            .asESPointer()
            ->asESObject()
        {% endset %}
        {% set module_object %}
    {{ name }}Function
            ->asESObject()
        {% endset %}
    ESValue {{ attribute.name }}Value = ESValue({{ attribute.value }});
    {{ bind_const(prototype_object|trim, attribute) }}
    {{ bind_const(module_object|trim, attribute) }}
    {% else %}
        {% if attribute.setter %}
            {% if attribute.custom_setter %}
                {% set setter %}
                    {{ attribute.name }}SetterFunction
                {% endset %}
            {% else %}
                {% set setter %}
                    {{ attribute.name }}{{ name }}SetterFunction
                {% endset %}
            {% endif %}
        {% else %}
            {% set setter %}nullptr{% endset %}
        {% endif %}
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        {{ name }}Function->protoType().asESPointer()->asESObject(),
        {{ attribute.name }}String,
        {% if attribute.custom_getter %}
        {{ attribute.name }}{{ name }}GetterFunction,
        {% else %}
        {{ attribute.name }}GetterFunction,
        {% endif %}
        {{ setter|trim }});
    {% endif %}
{% endwith %}
{% endcall %}