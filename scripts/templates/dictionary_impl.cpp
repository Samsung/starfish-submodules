{% import 'util.cpp' as util_macro %}
{% if dictionary.flags and dictionary.flags|length > 0 %}
#if defined({{dictionary.flags[0]}})
    {%- for idx in range(1, dictionary.flags|length) %}
        {{-  ' && defined(%s)'|format(dictionary.flags[idx]) -}}
    {% endfor %}
{% endif %}
extern {{dictionary.name}} to{{dictionary.name}}FromValueRef(ExecutionStateRef* state, ValueRef* from);
extern ValueRef* toValueRefFrom{{dictionary.name}}(ExecutionStateRef* state, {{dictionary.name}}& from);
{% if dictionary.flags and dictionary.flags|length > 0 %}
#endif
{% endif %}


