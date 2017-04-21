{% macro gen_bind(object, constant) %}
{{ object }}
            ->defineDataProperty({{ constant.name }}String,
                                 false, true, false,
                                 {{ constant.name }}Value);
{% endmacro %}