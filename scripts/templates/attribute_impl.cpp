{% import 'util.cpp' as util_macro %}
{% import 'util_for_attribute.cpp' as util_for_attribute_macro %}

{%- call util_macro.ifdef(attribute.flags) %}
{% with %}
{% if attribute.getter %}
{% if attribute.custom_getter %}
extern ESValue {{ util_for_attribute_macro.getter_function(attribute, name) }}(ESVMInstance* instance);
{% else %}
static ESValue {{ util_for_attribute_macro.getter_function(attribute, name) }}(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE({{ name }});
    {% set need_catch = attribute.getter.raises_exception %}
    {% set indent_callexp = 4 if need_catch else 0 %}
    {% set getter_type = attribute.getter.return %}
    {{ util_macro.gen_declare_return_value(attribute.getter.return)|trim }}
    {% if need_catch %}
    try {
    {% endif %}
    {{ 'result = originalObj->%s();'|format(util_macro.attr_name(attribute))|indent(indent_callexp, True) }}
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
{% if attribute.setter %}

{% if attribute.custom_setter %}
extern ESValue {{ util_for_attribute_macro.setter_function(attribute, name) }}(ESVMInstance* instance);
{% else %}
static ESValue {{ util_for_attribute_macro.setter_function(attribute, name) }}(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE({{ name }});
    {% set need_catch = attribute.setter.raises_exception %}
    {% set indent_callexp = 4 if need_catch else 0 %}
    {% set arg = attribute.setter.arguments[0] %}
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    {{ util_macro.handle_arg(arg, 'arg0', 'value0')|trim }}
    {{- 'Error : Wrong argument type' | assert_true(arg.type.name in ['void']) }}
    {{- 'Error : Unimplemented argument type' | assert_true(arg.type.name in ['object', 'Sequence', 'UnionType', 'Promise']) }}
    {% if need_catch %}
    try {
    {% endif %}
    {{ 'originalObj->set%s(value0);'|format(util_macro.attr_name(attribute)|first_word_capitalize)|indent(indent_callexp, True) }}
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