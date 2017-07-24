{% if strict_mode %}
{% import 'util.cpp' as util_macro %}
{%- call util_macro.ifdef(constant.flags) %}
    StringRef* {{ constant.name }}String = StringRef::fromASCII("{{ constant.name }}");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        state,
        targetObject, {{ constant.name }}String,
        {{ constant.name }}Unimplemented,
        nullptr, true, false);
{%- endcall %}
{% endif %}
