{% import 'util.cpp' as util_macro %}

{%- call util_macro.ifdef(attribute.flags) %}
    ESString* {{ attribute.name }}String = ESString::create("{{ attribute.name }}");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        {{ util_macro.gen_property_owner(primary_global, attribute.unforgeable, name) }}, {{ attribute.name }}String,
        {{ util_macro.gen_getter_function(attribute, name) }},
        {{ util_macro.gen_setter_function(attribute, name) if attribute.setter else 'nullptr' }},
        true/* enumerable */, {{ 'false' if attribute.unforgeable else 'true' }}/* configurable */);
{% endcall %}