{# TODO replace 'temp_util_for_function.cpp' to 'util.cpp' #}
{% import 'temp_util_for_function.cpp' as util_macro %}

static {{dictionary.name}} to{{dictionary.name}}FromESValue(ESValue from)
{
{% for key in dictionary['keys'] %}
    ESValue arg{{loop.index - 1}} = from.asESPointer()->asESObject()->get(ESString::create("{{ key.name }}"));
{% endfor %}
{% for key in dictionary['keys'] %}
    {{ util_macro.handle_arg(key, 'arg%d'|format(loop.index - 1), 'value%d'|format(loop.index - 1)) -}}
{% endfor %}
    return {{dictionary.name}}({{'value'|to_arg_syntax(0, dictionary['keys']|length)}});
}

static ESValue toESValueFrom{{dictionary.name}}({{dictionary.name}} from)
{
    // NOTE Please let me know when this function needed (ji.yang)
    return ESValue(ESValue::ESNull);
}

