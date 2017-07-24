{%- if not function.name == '_unnamed_' and (not function.unimplemented or strict_mode) %}
{% import 'util.cpp' as util_macro %}
{% call util_macro.ifdef(function.flags) %}
    {% set fn_name = '%s%s'|format(function.name, name)
                      if function.custom else function.name %}
    StringRef* {{ function.name }}String = StringRef::fromASCII("{{ function.name }}");
    {% if function.static %}
        {% set object %}{{ name }}Function{% endset %}
    {% else %}
        {% set object %}targetObject{% endset %}
    {% endif %}
    {% set writable = 'false' if function.unforgeable else 'true' %}
    {% set enumerable = 'true' %}
    {% set configurable = 'false' if function.unforgeable else 'true' %}
    {% if function.force_deny_strict %}
        {% set fnStrict %}false{% endset %}
    {% else %}
        {% set fnStrict %}true{% endset %}
    {% endif %}
    FunctionObjectRef* {{fn_name}}ESFn = FunctionObjectRef::create(state,
                            FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "{{ function.name }}"), {{ fn_name }}Function, {{ function.min_passed_count|default(0) }}, nullptr, {{ fnStrict }}, false)
                            );
    {{ object }}->defineDataProperty(state,
                            ValueRef::create({{ function.name }}String),
                            ValueRef::create({{fn_name}}ESFn),
                            {{ writable }}/* writable */, {{ enumerable }}/* enumerable */, {{ configurable }}/* configurable */
                            );
{% endcall %}
{% endif %}
