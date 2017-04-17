{% import 'util.cpp' as util_macro %}
{% import 'util_for_attribute.cpp' as util_for_attribute_macro %}

{%- call util_macro.ifdef(attribute.flag) %}
    ESString* {{ attribute.name }}String = ESString::create("{{ attribute.name }}");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        {{ name }}Function->protoType().asESPointer()->asESObject(),
        {{ attribute.name }}String,
        {{ util_for_attribute_macro.getter_function(attribute, name) }},
        {{ util_for_attribute_macro.setter_function(attribute, name) }});
{% endcall %}