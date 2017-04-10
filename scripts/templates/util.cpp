{% macro ifdef(flag) -%}
  {% if flag %}
#ifdef {{ flag }}
{{ caller() }}
#endif
  {% else %}
{{ caller() }}
  {% endif %}
{%- endmacro %}
