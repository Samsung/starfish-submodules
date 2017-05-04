{% import 'util.cpp' as util_macro %}

{%- macro gen_call_getter(vname, attribute, indent) -%}
{{ '%s = originalObj->%s();'|format(vname, util_macro.gen_attr_name(attribute))|indent(indent, True) }}
{%- endmacro -%}

{%- macro gen_call_setter(vname, attribute, indent) -%}
{{ '%s->set%s(value0);'|format(vname, util_macro.gen_attr_name(attribute)|first_word_capitalize)|indent(indent, True) }}
{%- endmacro -%}

{%- macro gen_call_setter_if_non_null(vname, attribute, indent) -%}
    if (forwards) {
    {{ gen_call_setter(vname, attribute, 4) }}
    }
{% endmacro %}

{%- call util_macro.ifdef(attribute.flags) %}
{% with %}
{% if attribute.getter %}
{% if attribute.getter.custom %}
extern ESValue {{ util_macro.gen_getter_function(attribute, name) }}(ESVMInstance* instance);
{% else %}
static ESValue {{ util_macro.gen_getter_function(attribute, name) }}(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE({{ name }});
    {% if attribute.getter.return.name == 'EventHandlerNonNull' %}
        {# ignore x it is just a trick to set entry on dict type #}
        {% set x=attribute.getter.return.__setitem__('name', 'EventListener') %}
    {% endif %}
    {% set need_catch = attribute.getter.raises_exception %}
    {% set indent_callexp = 4 if need_catch else 0 %}
    {% set getter_type = attribute.getter.return %}
    {{ util_macro.gen_declare_return_value(attribute.getter.return)|trim }}
    {% if need_catch %}
    try {
    {% endif %}
    {{ gen_call_getter('result', attribute, indent_callexp) }}
    {% if need_catch %}
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    {% endif %}
    {{ util_macro.handle_return(attribute.getter.return)|trim }}
    {{- 'Error : Unimplemented return type' | assert_true(attribute.getter.return.name in ['object', 'Sequence', 'UnionType', 'Promise']) }}
}
{% endif %}
{% endif %}
{% if attribute.put_forwards %}
    {% set x=attribute.__setitem__('setter', attribute.put_forwards.setter) %}
{% endif %}
{% if attribute.setter %}

{% if attribute.setter.custom %}
extern ESValue {{ util_macro.gen_setter_function(attribute, name) }}(ESVMInstance* instance);
{% else %}
static ESValue {{ util_macro.gen_setter_function(attribute, name) }}(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE({{ name }});
    {% if attribute.setter.return.name == 'EventHandlerNonNull' %}
        {% set x=attribute.setter.return.__setitem__('name', 'EventListener') %}
    {% endif %}
    {% set need_catch = attribute.setter.raises_exception %}
    {% set indent_callexp = 4 if need_catch else 0 %}
    {% set arg = attribute.setter.arguments[0] %}
    {% set names = {'name': name, 'attrname': attribute.name, 'aname': 'arg0', 'vname': 'value0'} %}
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    {{ util_macro.handle_arg(arg, names, fromattr=True)|trim }}
    {{- 'Error : Wrong argument type' | assert_true(arg.type.name in ['void']) }}
    {{- 'Error : Unimplemented argument type' | assert_true(arg.type.name in ['object', 'Sequence', 'UnionType', 'Promise']) }}
    {% if need_catch %}
    try {
    {% endif %}
    {% if attribute.put_forwards %}
    {{ util_macro.gen_declare_return_value(attribute.getter.return, 'forwards')|trim|indent(indent_callexp, True) }}
    {{ gen_call_getter('forwards', attribute, indent_callexp) }}
    {{ gen_call_setter_if_non_null('forwards', attribute.put_forwards)|indent(indent_callexp, True) }}
    {# TODO: what if forwards is nullptr #}
    {% else %}
    {{ gen_call_setter('originalObj', attribute, indent_callexp) }}
    {% endif %}
    {% if need_catch %}
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    {% endif %}
    return ESValue();
}
{% endif %}
{% endif %}
{% endwith %}
{% endcall %}