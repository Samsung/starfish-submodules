 {% if constructor and not constructor.unimplemented and constructor.name|length > 0 %}

ESFunctionObject* binding{{ constructor.name }}(
    ScriptBindingInstance* scriptBindingInstance)
{
    ESString* {{ constructor.name }}String = ESString::create("{{ constructor.name }}");

    ESFunctionObject* {{constructor.name}}Function =
        ESFunctionObject::create(nullptr,
                                 {{ name|lower }}Constructor,
                                 {{ constructor.name }}String,
                                 {{ constructor.arguments|length|default(0) }}, true, true);

    {{ constructor.name }}Function->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(),
        false, false, false);
    {{ constructor.name }}Function->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    {{ constructor.name }}Function->protoType()
        .asESPointer()
        ->asESObject()
        ->set__proto__(fetchData(scriptBindingInstance)->fn{{ name }}()->protoType());
    {{ constructor.name }}Function->set__proto__(fetchData(scriptBindingInstance)->fn{{ name }}());
    return {{ constructor.name }}Function;
}
{% endif %}