{% macro prototype_object(name) %}
{{ name }}Function->protoType()
            .asESPointer()
            ->asESObject()
{%- endmacro %}

{% macro function_object(name) %}
{{ name }}Function
            ->asESObject()
{%- endmacro %}

{% macro gen_bind(object, constant) %}
{{ object }}
            ->defineDataProperty({{ constant.name }}String,
                                 false,
                                 true,
                                 false,
                                 {{ constant.name }}Value);
{% endmacro %}