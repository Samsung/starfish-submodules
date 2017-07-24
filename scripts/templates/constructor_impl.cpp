{% if constructor.unimplemented %}
{% include 'unimpl_constructor_impl.cpp' ignore missing %}
{% else %}
{% import 'util.cpp' as util_macro %}
{% if constructor.custom %}
extern ValueRef* {{ name|lower }}Constructor(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression);

{% else %}
static ValueRef* {{ name|lower }}Constructor(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    {% set max_arg = constructor.arguments|length %}
    {% set min_passing_count = constructor.min_passing_count|default(0) %}
    {% set min_passed_count = constructor.min_passed_count|default(0) %}
    {% set uniformed_call = (max_arg == min_passing_count) %}
    {% set need_counting = (not uniformed_call) and max_arg - constructor.min_passed_count > 1 %}
    if (!isNewExpression) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "{{ name }}");
        THROW_EXCEPTION(msg);
    }
    {% if min_passed_count != 0 %}
    size_t argCount = argc;
    if (argCount < {{ min_passed_count }}) {
        {% set siz = min_passed_count|digit + 1 %}
        char buffer[{{ siz }}];
        snprintf(buffer, {{ siz }}, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "{{ min_passed_count }}", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_CONSTRUCT, "parseFromString", "{{ name }}", reason);
        THROW_EXCEPTION(msg);
    }
    {% endif %}
    {% if not uniformed_call %}
    size_t validArgCount = {{ constructor.arguments|length }};
    {% endif %}
    {% if need_counting %}
    bool argCounting = true;
    {% endif %}
    {% for arg in constructor.arguments %}
        {% if loop.index - 1 < min_passed_count %}
    ValueRef* arg{{loop.index - 1}} = argv[{{loop.index - 1}}];
        {% else %}
    ValueRef* arg{{loop.index - 1}} = (argc > {{loop.index - 1}}) ? argv[{{loop.index - 1}}] : ValueRef::createUndefined();
        {% endif %}
    {% endfor %}
    {% for idx in range(0, max_arg) %}
    {% set ridx = max_arg - idx - 1 -%}
    {% set names = {'name': name, 'aname': 'arg%d'|format(ridx), 'vname': 'value%d'|format(ridx)} %}
    {{ util_macro.handle_arg(constructor.arguments[ridx], names, need_counting=need_counting)|trim }}
    {% endfor %}
    {{ name }}* result = nullptr;
    {% set call_with = 'callWith' if constructor.call_with else '' %}
    {% set call_with_comma = 'callWith, ' if call_with|length > 0 else '' %}
    {% if constructor.call_with  == 'Document' %}
    Document* callWith = fetchDocument(state->context());
    {% elif constructor.call_with  == 'Starfish' %}
    StarFish* callWith = fetchStarFish(state->context());
    {% elif constructor.call_with  == 'Window' %}
    Window* callWith = fetchWindow(state->context());
    {% endif %}
    // Call native function (nargs: {{max_arg if uniformed_call else '%s-%s'|format(min_passing_count, max_arg)}})
    {% if max_arg == 0 %}
    result = new {{name}}({{call_with}});
    {% elif uniformed_call %}
    result = new {{name}}({{call_with_comma}}{{ 'value'|to_arg_syntax(0, max_arg) }});
    {% else %}
    if (validArgCount == {{min_passing_count|string}}) {
        {% if min_passing_count == 0 %}
        result = new {{name}}({{call_with}});
        {% else %}
        result = new {{name}}({{call_with_comma}}{{'value'|to_arg_syntax(0, min_passing_count)}});
        {% endif %}
        {% for count in range(min_passing_count + 1, max_arg + 1) %}
    } else if (validArgCount == {{count|string}}) {
        result = new {{name}}({{call_with_comma}}{{'value'|to_arg_syntax(0, count)}});
        {% endfor %}
    }
    {% endif %}
    return result->scriptValue();
}

{% endif %}
{% endif %}
