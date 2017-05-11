{% import 'util.cpp' as util_macro %}

{%- call util_macro.ifdef(constant.flags) %}
    ESString* {{ constant.name }}String = ESString::create("{{ constant.name }}");
    ESValue {{ constant.name }}Value = ESValue({{ constant.value }});
    {{ name }}PrototypeObj
            ->defineDataProperty({{ constant.name }}String,
                                 false/* writable */, true/* enumerable */, false/* configurable */,
                                 {{ constant.name }}Value);
    {{ name }}Function
            ->defineDataProperty({{ constant.name }}String,
                                 false/* writable */, true/* enumerable */, false/* configurable */,
                                 {{ constant.name }}Value);
{% endcall %}