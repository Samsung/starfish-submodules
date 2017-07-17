    StringRef* {{ name }}String = StringRef::fromASCII("{{ name }}");
    {% if descriptor %}
        {% set native_ctor_fn %}TODO_UNFINISHED_CODE(ExecutionStateRef* state, size_t argc, ValueRef** argv){% endset %}
    {% else %}
        {% set native_ctor_fn %}nullptr{% endset %}
    {% endif %}
    {% if constructor and constructor.name|length == 0 and not constructor.unimplemented %}
    FunctionObjectRef::NativeFunctionInfo ctorInfo(AtomicStringRef::create(context, "{{ name }}"), {{ name|lower }}Constructor, {{ constructor.min_passed_count|default(0) }}, {{ native_ctor_fn }}, true, true);
    {% else %}
    FunctionObjectRef::NativeFunctionInfo ctorInfo(AtomicStringRef::create(context, "{{ name }}"), errorOnConstructorFunction, 0, nullptr, true, true);
    {% endif %}
    FunctionObjectRef* {{ name }}Function = FunctionObjectRef::createBuiltinFunction(state, ctorInfo);
    ObjectRef* {{ name }}PrototypeObj = {{ name }}Function->getFunctionPrototype(state)->asObject();
    {{ name }}PrototypeObj->removeFromHiddenClassChain(state);
    {% if parent %}
        {% set parent_class %}
    ValueRef::create(scriptBindingInstance
                ->fn{{ parent.name }}()
                ->getFunctionPrototype(state))
        {% endset %}
    {% elif constructor and constructor.prototype == 'Error' %}
        {% set parent_class %}
    ValueRef::create(scriptBindingInstance
            ->scriptContext()
            ->globalObject()
            ->errorPrototype())
        {% endset %}
    {% else %}
        {% set parent_class %}
    ValueRef::create(scriptBindingInstance
            ->scriptContext()
            ->globalObject()
            ->objectPrototype())
        {% endset %}
    {% endif %}
    {{ name }}PrototypeObj->setPrototype(state, {{ parent_class|trim }});
    {% if parent %}
    {{ name }}Function->setPrototype(state, ValueRef::create(scriptBindingInstance
            ->fn{{ parent.name }}()));
    {% endif %}
