{% import 'util.cpp' as util_macro %}

{%- call util_macro.ifdef(attribute.flags) %}
    StringRef* {{ attribute.name }}String = StringRef::fromASCII("{{ attribute.name }}");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        state,
        {{ util_macro.gen_property_owner(primary_global, attribute.unforgeable, name) }}, {{ attribute.name }}String,
        {{ util_macro.gen_getter_function(attribute, name) }},
        {{ util_macro.gen_setter_function(attribute, name) if attribute.setter else 'nullptr' }},
        true/* enumerable */, {{ 'false' if attribute.unforgeable else 'true' }}/* configurable */);
{% endcall %}
