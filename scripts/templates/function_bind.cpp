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
            {{ name }}Obj
        {% endset %}
    {% endif %}
    {% set writable = function.writable|default(True)|lower %}
    {% set enumerable = function.enumerable|default(True)|lower %}
    {% set configurable = function.configurable|default(True)|lower %}
    ESFunctionObject* {{fn_name}}ESFn = ESFunctionObject::create(
                            nullptr,
                            {{ fn_name }}Function,
                            {{ function.name }}String,
                            {{ function.arguments|length|default(0) }}, false);
    {{ object|trim }}->defineDataProperty(
                            {{ function.name }}String,
                            {{ writable }}, {{ enumerable }}, {{ configurable }},
                            {{fn_name}}ESFn);
{% endcall %}