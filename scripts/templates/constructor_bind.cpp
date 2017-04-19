    ESString* {{ name }}String = ESString::create("{{ name }}");
    {% if constructor and constructor.name|length == 0 and not constructor.unimplemented %}
    ESFunctionObject* {{ name }}Function =
        ESFunctionObject::create(nullptr,
                                 {{ name|lower }}Constructor,
                                 {{ name }}String,
                                 {{ constructor.min_passed_count|default(0) }}, true, true);
    {% else %}
    ESFunctionObject* {{ name }}Function =
        ESFunctionObject::create(nullptr,
                                 errorOnConstructorFunction,
                                 {{ name }}String,
                                 0, true, true);
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
                ->fn{{ parent }}()
                ->protoType()
        {% endset %}
    {% elif constructor and constructor.prototype == 'Error' %}
        {% set parent_class %}
    fetchData(scriptBindingInstance)
            ->m_instance
            ->globalObject()
            ->errorPrototype()
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
        ->set__proto__({{ parent_class|trim }});
    {% if parent %}
    {{ name }}Function->set__proto__(fetchData(scriptBindingInstance)
            ->fn{{ parent }}());
    {% endif %}
    {% if functions and functions|length > 0 %}
    ESObject* {{ name }}Obj = {{ name }}Function->protoType().asESPointer()->asESObject();
    {% endif %}
