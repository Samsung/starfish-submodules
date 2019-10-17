{% import 'util.cpp' as util_macro %}
{% if strict_mode %}
{%- call util_macro.ifdef(attribute.flags) %}
    StringRef* {{ attribute.name }}String = StringRef::createFromASCII("{{ attribute.name }}");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        state,
        targetObject, {{ attribute.name }}String,
        {{ attribute.name }}UnimplementedFunction,
        {{ attribute.name }}UnimplementedFunction, true, false);
{% endcall %}
{% endif %}
