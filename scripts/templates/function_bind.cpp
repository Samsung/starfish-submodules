{% import 'util.cpp' as util_macro %}
{%- if not function.name == '_unnamed_' %}
{% call util_macro.ifdef(function.flags) %}
    {% set fn_name = '%s%s'|format(function.name, name)
                      if function.custom else function.name %}
    ESString* {{ function.name }}String = ESString::create("{{ function.name }}");
    {% if function.static %}
        {% set object %}{{ name }}Function{% endset %}
    {% else %}
        {% set object %}{{ util_macro.gen_property_owner(primary_global, function.unforgeable, name) }}{% endset %}
    {% endif %}
    {% set writable = 'false' if function.unforgeable else 'true' %}
    {% set enumerable = 'true' %}
    {% set configurable = 'false' if function.unforgeable else 'true' %}
    ESFunctionObject* {{fn_name}}ESFn = ESFunctionObject::create(
                            nullptr,
                            {{ fn_name }}Function,
                            {{ function.name }}String,
                            {{ function.min_passed_count|default(0) }}, false);
    {% if function.force_deny_strict %}
    {{fn_name}}ESFn->codeBlock()->m_forceDenyStrictMode = true;
    {% endif %}
    {{ object }}->defineDataProperty(
                            {{ function.name }}String,
                            {{ writable }}/* writable */, {{ enumerable }}/* enumerable */, {{ configurable }}/* configurable */,
                            {{fn_name}}ESFn);
{% endcall %}
{% endif %}