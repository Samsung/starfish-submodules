{% import 'util.cpp' as util_macro %}
{% import 'util_for_constant.cpp' as util_for_constant_macro %}

{%- call util_macro.ifdef(constant.flags) %}
    ESString* {{ constant.name }}String = ESString::create("{{ constant.name }}");
    ESValue {{ constant.name }}Value = ESValue({{ constant.value }});
    {{ util_for_constant_macro.gen_bind('%sPrototypeObj'|format(name), constant) }}
    {{ util_for_constant_macro.gen_bind('%sFunction'|format(name), constant) -}}
{% endcall %}