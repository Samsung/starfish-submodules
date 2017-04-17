{%- macro getter_function(attribute, name) -%}
    {% if attribute.custom_getter %}
        {{- '%s%sGetterFunction'|format(attribute.name, name) -}}
    {% else %}
        {{- '%sGetterFunction'|format(attribute.name) -}}
    {% endif %}
{%- endmacro -%}

{%- macro setter_function(attribute, name) -%}
    {% if attribute.setter %}
        {% if attribute.custom_setter %}
            {{- '%s%sSetterFunction'|format(attribute.name, name) -}}
        {% else %}
            {{- '%sSetterFunction'|format(attribute.name) -}}
        {% endif %}
    {% else %}
        {{- 'nullptr' -}}
    {% endif %}
{%- endmacro -%}
