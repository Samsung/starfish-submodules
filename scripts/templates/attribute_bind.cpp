{% import 'util.cpp' as util_macro %}

{%- call util_macro.ifdef(attribute.flags) %}
    ESString* {{ attribute.name }}String = ESString::create("{{ attribute.name }}");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        {{ name }}PrototypeObj, {{ attribute.name }}String,
        {{ util_macro.gen_getter_function(attribute, name) }},
        {{ util_macro.gen_setter_function(attribute, name) if attribute.setter else 'nullptr' }});
{% endcall %}