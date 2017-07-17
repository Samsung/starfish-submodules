{% import 'util.cpp' as util_macro %}

{%- call util_macro.ifdef(constant.flags) %}
    StringRef* {{ constant.name }}String = StringRef::fromASCII("{{ constant.name }}");
    ValueRef* {{ constant.name }}Value = ValueRef::create({{ constant.value }});
    targetObject
            ->defineDataProperty(state, ValueRef::create({{ constant.name }}String),
                                 {{ constant.name }}Value,
                                 false/* writable */, true/* enumerable */, false/* configurable */
                                 );
    {% if not constant.unforgeable and not primary_global %}
    {{ name }}Function
            ->defineDataProperty(state, ValueRef::create({{ constant.name }}String),
                                 {{ constant.name }}Value,
                                 false/* writable */, true/* enumerable */, false/* configurable */
                                  );
    {% endif %}
{% endcall %}
