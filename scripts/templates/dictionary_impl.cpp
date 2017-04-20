{# TODO replace 'temp_util_for_function.cpp' to 'util.cpp' #}
{% import 'util.cpp' as util_macro %}
{% if dictionary.flags and dictionary.flags|length > 0 %}
#if defined({{dictionary.flags[0]}})
    {%- for idx in range(1, dictionary.flags|length) %}
        {{-  ' && defined(%s)'|format(dictionary.flags[idx]) -}}
    {% endfor %}
{% endif %}
extern {{dictionary.name}} to{{dictionary.name}}FromESValue(ESVMInstance* instance, ESValue& from);
extern ESValue toESValueFrom{{dictionary.name}}(ESVMInstance* instance, {{dictionary.name}}& from);
{% if dictionary.flags and dictionary.flags|length > 0 %}
#endif
{% endif %}


