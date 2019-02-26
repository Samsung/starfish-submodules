{% import 'util.cpp' as util_macro %}
{% if dictionary.flags and dictionary.flags|length > 0 %}

{% for flag in dictionary.flags %}
#ifdef {{ flag }}
{% endfor %}
{% endif %}
extern {{dictionary.name}} to{{dictionary.name}}FromValueRef(ExecutionStateRef* state, ValueRef* from);
extern ValueRef* toValueRefFrom{{dictionary.name}}(ExecutionStateRef* state, {{dictionary.name}}& from);
{% if dictionary.flags and dictionary.flags|length > 0 %}
#endif
{% endif %}


