{% import 'util.cpp' as util_macro %}
{% import 'util_for_attribute.cpp' as util_for_attribute_macro %}

{%- call util_macro.ifdef(attribute.flags) %}
    ESString* {{ attribute.name }}String = ESString::create("{{ attribute.name }}");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        {{ name }}PrototypeObj, {{ attribute.name }}String,
        {{ util_for_attribute_macro.getter_function(attribute, name) }},
        {{ util_for_attribute_macro.setter_function(attribute, name) }});
{% endcall %}