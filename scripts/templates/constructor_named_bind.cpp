 {% if constructor and not constructor.unimplemented and constructor.name|length > 0 %}

FunctionObjectRef* binding{{ constructor.name }}(
    ScriptBindingInstance* scriptBindingInstance)
{
    ContextRef* context = scriptBindingInstance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(context);

    {% if object_type == "exposable" %}
        {% set native_ctor_fn %}{{ name }}Constructor(ExecutionStateRef* state, size_t argc, ValueRef** argv){% endset %}
    {% else %}
        {% set native_ctor_fn %}nullptr{% endset %}
    {% endif %}

    FunctionObjectRef::NativeFunctionInfo ctorInfo(AtomicStringRef::create(context, "{{ constructor.name }}"), {{ name|lower }}Constructor, {{ constructor.arguments|length|default(0) }}, {{ native_ctor_fn }}, true, true);

    FunctionObjectRef* {{ constructor.name }}Function = FunctionObjectRef::createBuiltinFunction(state, ctorInfo);

    {{ constructor.name }}Function->removeFromHiddenClassChain(state);

    {{ constructor.name }}Function->getFunctionPrototype(state)->asObject()
        ->setPrototype(state, scriptBindingInstance->fn{{ name }}()->getFunctionPrototype(state));
    {{ constructor.name }}Function->setPrototype(state, ValueRef::create(scriptBindingInstance->fn{{ name }}()));

    state->destroy();
    return {{ constructor.name }}Function;
}
{% endif %}
