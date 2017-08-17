{% import 'util.cpp' as util_macro %}
{% set igetter = descriptor.indexed_getter %}
{% set ngetter = descriptor.named_getter %}

{%- macro handle_getter(getter, type) %}

    {% set getter_name = 'default%s'|format(type) if getter.name == '_unnamed_' else getter.name %}
    {% set use_nullable = getter.return.kind in nullable_kinds %}
    {% set result_name = 'result.getValue()' if use_nullable else 'result' %}
    {% set return_stm = 'return ExposableObjectGetOwnPropertyCallbackResult(%s, true, false, false);'|format(util_macro.gen_native_to_jsvalue(getter.return, result_name)) %}
    // {{type}}
    {% if type == 'NamedGetter' %}
    // Search matched property through prototype chain
    ObjectRef* ref = jsSelf->getPrototypeObject();
    while (ref) {
        if (ref->hasOwnProperty(state, key)) {
            return ExposableObjectGetOwnPropertyCallbackResult();
        }
        ref = ref->getPrototypeObject();
    }
    {% endif %}
    {{ util_macro.gen_declare_return_value(getter.return, is_descriptor=True)|trim }}
    {% if type == 'IndexedGetter' %}
    result = self->{{getter_name}}(idx);
    {% else %}
    result = self->{{getter_name}}(toBrowserString(state, key));
    {% endif %}
    {% if use_nullable %}
    if (result.hasValue()) {
        {{ return_stm }}
    }
    {% elif getter.return.kind in pointer_kinds %}
    if (result != nullptr) {
        {{ return_stm }}
    }
    {% else %}
    {{ return_stm }}
    {% endif %}
{% endmacro -%}

static ExposableObjectGetOwnPropertyCallbackResult {{ name }}GetOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* jsSelf, ValueRef* key)
{
{% if igetter or ngetter %}
    {{name}}* self = ({{name}}*)jsSelf->extraData();
    STARFISH_ASSERT(self->is{{name}}());
    {% if igetter and ngetter %}
    uint32_t idx = key->toArrayIndex(state);
    if (idx == ValueRef::InvalidArrayIndexValue) {
        {{- handle_getter(ngetter, 'NamedGetter')|indent(4, False) }}
    } else if (idx < self->length()) {
        {{- handle_getter(igetter, 'IndexedGetter')|indent(4, False) }}
    }
    {% elif igetter %}
    uint32_t idx = key->toArrayIndex(state);
    if (idx != ValueRef::InvalidArrayIndexValue && idx < self->length()) {
        {{- handle_getter(igetter, 'IndexedGetter')|indent(4, False) }}
    }
    {% elif ngetter %}
    {{- handle_getter(ngetter, 'NamedGetter') }}
    {% endif %}
{% else %}
    // No getter found in {{ name }}
{% endif %}
    // NOTE Ignore this when there are multiple return statements in same depth
    return ExposableObjectGetOwnPropertyCallbackResult();
}

static bool {{ name }}DefineOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* jsSelf, ValueRef* key, ValueRef* value)
{
    {% if descriptor.setter %}
    {% set setter_name = 'defaultSetter' if descriptor.setter.name == '_unnamed_' else descriptor.setter.name %}
    {% set names = {'name': name, 'fname': 'Setter', 'aname': 'value', 'vname': 'valueTo'} %}
    {{name}}* self = ({{name}}*)jsSelf->extraData();
    STARFISH_ASSERT(self->is{{name}}());
    {{ util_macro.handle_arg(descriptor.setter.arguments[1], names)|trim }}
    self->{{setter_name}}(toBrowserString(state, key), valueTo);
    return true;
    {% else %}
    // No setter found in {{ name }}
    return false;
    {% endif %}
}

static ExposableObjectEnumerationCallbackResultVector {{ name }}EnumerationCallback(ExecutionStateRef* state, ObjectRef* jsSelf)
{
    {{name}}* self = ({{name}}*)jsSelf->extraData();
    STARFISH_ASSERT(self->is{{name}}());
    ExposableObjectEnumerationCallbackResultVector v;
    {% if igetter and igetter.enumerable %}
    size_t len = self->length();
    for (size_t i = 0; i < len; i++) {
        v.push_back(ExposableObjectEnumerationCallbackResult(
            ValueRef::create(i), false, true, false));
    }
    {% endif %}
    {% if ngetter and ngetter.enumerable %}
    std::vector<const char*> enums;
    self->defaultNamedEnumerator(enums);
    for (size_t i = 0; i < enums.size(); i++) {
        v.push_back(ExposableObjectEnumerationCallbackResult(
            ValueRef::create(StringRef::fromASCII(enums[i])), false, true, false));
    }
    {% endif %}
    return v;
}

