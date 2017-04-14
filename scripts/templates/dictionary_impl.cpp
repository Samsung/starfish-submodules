{# TODO replace 'temp_util_for_function.cpp' to 'util.cpp' #}
{% import 'util.cpp' as util_macro %}

static {{dictionary.name}} to{{dictionary.name}}FromESValue(ESVMInstance* instance, ESValue from)
{
	if (!from.isObject()) {
		auto msg = ESString::create("Failed to generate {{dictionary.name}} from non-object");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
	}
{% for key in dictionary['keys'] %}
    ESValue arg{{loop.index - 1}} = from.asESPointer()->asESObject()->get(ESString::create("{{ key.name }}"));
{% endfor %}
{% for key in dictionary['keys'] -%}
    {{ util_macro.handle_arg(key, 'arg%d'|format(loop.index - 1), 'value%d'|format(loop.index - 1)) }}
{% endfor %}
    return {{dictionary.name}}({{'value'|to_arg_syntax(0, dictionary['keys']|length)}});
}

static ESValue toESValueFrom{{dictionary.name}}(ESVMInstance* instance, {{dictionary.name}}& from)
{
    // NOTE Please let me know when this function needed (ji.yang)
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return ESValue(ESValue::ESNull);
}


