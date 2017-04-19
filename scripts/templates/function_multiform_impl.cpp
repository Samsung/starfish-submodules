{% import 'util.cpp' as util_macro %}
{% for function in function.operations %}
{% include 'function_impl.cpp' ignore missing %}
{% endfor %}
static ESValue {{ function.name }}Function(ESVMInstance* instance)
{
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (false) {
    {% for fn in function.operations %}
{% if fn.flags|length > 0 %}
#if defined({{fn.flags[0]}})
        {%- for idx in range(1, fn.flags|length) %}
            {{-  ' && defined(%s)'|format(fn.flags[idx]) -}}
        {% endfor %}

{% endif %}
		{% if fn.min_passed_count == fn.arguments|length %}
	} else if (argCount == {{fn.min_passed_count}}) {
		{% else %}
	} else if (argCount >= {{fn.min_passed_count}} && argCount <= {{fn.arguments|length}}) {
		{% endif %}
		{{ 'return %s%sFunction(instance);'|format(fn.name, fn.id) }}
{% if fn.flags %}
#endif
{% endif %}
	{% endfor %}
	} else {
		auto msg = ESString::create("Invalid arguments");
	    instance->throwError(ESValue(TypeError::create(msg)));
	    STARFISH_RELEASE_ASSERT_NOT_REACHED();
	}
}


