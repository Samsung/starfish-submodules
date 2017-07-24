{% import 'util.cpp' as util_macro %}
{% if attribute.unimplemented %}
{% include 'unimpl_attribute_bind.cpp' ignore missing %}
{% else %}
{%- call util_macro.ifdef(attribute.flags) %}
    StringRef* {{ attribute.name }}String = StringRef::fromASCII("{{ attribute.name }}");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        state,
        targetObject, {{ attribute.name }}String,
        {{ util_macro.gen_getter_function(attribute, name) }},
        {{ util_macro.gen_setter_function(attribute, name) if attribute.setter else 'nullptr' }},
        true/* enumerable */, {{ 'false' if attribute.unforgeable else 'true' }}/* configurable */);
{% endcall %}
{% endif %}
