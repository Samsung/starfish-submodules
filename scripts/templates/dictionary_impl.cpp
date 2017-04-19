{# TODO replace 'temp_util_for_function.cpp' to 'util.cpp' #}
{% import 'util.cpp' as util_macro %}
{% if flags and flags|length > 0 %}
#if defined({{flags[0]}})
    {%- for idx in range(1, flags|length) %}
        {{-  ' && defined(%s)'|format(flags[idx]) -}}
    {% endfor %}
{% endif %}
extern {{name}} to{{name}}FromESValue(ESVMInstance* instance, ESValue& from);
extern ESValue toESValueFrom{{name}}(ESVMInstance* instance, {{name}}& from);
{% if flags and flags|length > 0 %}
#endif
{% endif %}


