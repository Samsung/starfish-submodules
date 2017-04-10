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
            {% set setter %}
                {{ attribute.name }}SetterFunction
            {% endset %}
        {% else %}
            {% set setter %}nullptr{% endset %}
        {% endif %}
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        {{ name }}Function->protoType().asESPointer()->asESObject(),
        {{ attribute.name }}String,
        {{ attribute.name }}GetterFunction,
        {{ setter|trim }});
    {% endif %}
{% endcall %}