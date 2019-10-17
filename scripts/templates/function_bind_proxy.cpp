{%- if not function.name == '_unnamed_' and (not function.unimplemented or strict_mode) %}
{% import 'util.cpp' as util_macro %}
{% call util_macro.ifdef(function.flags) %}
    {% set fn_name = '%s%s'|format(function.name, name)
                      if function.custom else function.name %}
    {% set fn_static = function.static if not function.operations else function.operations[0].static %}
    StringRef* {{ function.name }}String = StringRef::createFromASCII("{{ function.name }}");
    {% if fn_static %}
        {% set object %}{{ name }}Function{% endset %}
    {% else %}
        {% set object %}targetObject{% endset %}
    {% endif %}
    {% set writable = 'false' if function.unforgeable else 'true' %}
    {% set enumerable = 'true' %}
    {% set configurable = 'false' if function.unforgeable else 'true' %}
    FunctionObjectRef* {{fn_name}}ESFn = windowObject->getOwnProperty(state, ValueRef::create({{ function.name }}String))->asFunction();
    {{ object }}->defineDataProperty(state,
                            ValueRef::create({{ function.name }}String),
                            ValueRef::create({{fn_name}}ESFn),
                            {{ writable }}/* writable */, {{ enumerable }}/* enumerable */, {{ configurable }}/* configurable */
                            );
{% endcall %}
{% endif %}
