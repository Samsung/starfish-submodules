    ESString* {{ name }}String = ESString::create("{{ name }}");
    {% if constructor and not constructor.unimplemented%}
    ESFunctionObject* {{ name }}Function = ESFunctionObject::create(
        nullptr, {{ name|lower }}Constructor, {{ name }}String,
        {{ constructor.length|default(0) }}, true, true);
    {% else %}
    ESFunctionObject* {{ name }}Function =
        ESFunctionObject::create(nullptr,
                                 errorOnConstructorFunction,
                                 {{ name }}String,
                                 1, true, true);
    {% endif %}

    {{ name }}Function->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(),
        false, false, false);

    {{ name }}Function->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);

    {% if parent %}
      {% set parent_class %}
    fetchData(scriptBindingInstance)
                ->{{ parent }}()
                ->protoType()
      {% endset %}
    {% elif error_class %}
      {% set parent_class %}
    fetchData(scriptBindingInstance)
                ->m_instance
                ->globalObject()
                ->errorPrototype()
      {% endset %}
    {% elif array_class %}
      {% set parent_class %}
    fetchData(scriptBindingInstance)
                ->m_instance
                ->globalObject()
                ->arrayPrototype()
      {% endset %}
    {% else %}
      {% set parent_class %}
    fetchData(scriptBindingInstance)
                ->m_instance
                ->globalObject()
                ->objectPrototype()
      {% endset %}
    {% endif %}
    {{ name }}Function->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(
            {{ parent_class|trim }});

    {% if parent %}
    {{ name }}Function->set__proto__(
        fetchData(scriptBindingInstance)->{{ parent }}());
    {% endif %}