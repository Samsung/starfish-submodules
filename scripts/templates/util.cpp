{% macro ifdef(flags) -%}
{% if flags %}
    {% for flag in flags %}
#ifdef {{ flag }}
    {% endfor %}
{{ caller()|trim }}
    {% for flag in flags %}
#endif
    {% endfor %}

  {% else %}
{{ caller() }}
{% endif %}
{%- endmacro %}
