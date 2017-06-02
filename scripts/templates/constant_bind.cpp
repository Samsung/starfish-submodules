{% import 'util.cpp' as util_macro %}

{%- call util_macro.ifdef(constant.flags) %}
    StringRef* {{ constant.name }}String = StringRef::fromASCII("{{ constant.name }}");
    ValueRef* {{ constant.name }}Value = ValueRef::create({{ constant.value }});
    {{ name }}PrototypeObj
            ->defineDataProperty(state, ValueRef::create({{ constant.name }}String),
                                {{ constant.name }}Value,
                                 false/* writable */, true/* enumerable */, false/* configurable */
                                 );
    {{ name }}Function
            ->defineDataProperty(state, ValueRef::create({{ constant.name }}String),
                                 {{ constant.name }}Value,
                                 false/* writable */, true/* enumerable */, false/* configurable */
                                  );
{% endcall %}
