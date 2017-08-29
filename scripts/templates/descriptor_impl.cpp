{% import 'util.cpp' as util_macro %}
{% set igetter = descriptor.indexed_getter %}
{% set ngetter = descriptor.named_getter %}
{% set isetter = descriptor.indexed_setter %}
{% set nsetter = descriptor.named_setter %}
{% set ideleter = descriptor.indexed_deleter %}
{% set ndeleter = descriptor.named_deleter %}

{%- macro raises_exception(exist, indent) %}
{% if exist %}
{{ 'try {
%s} catch (DOMException* e) {
    state->throwException(e->scriptValue());
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}'|format(caller())|indent(indent, true) }}
{% else %}
{{ caller() }}
{% endif %}
{%- endmacro -%}

{%- macro handle_getter(getter, type) %}

    {% set getter_name = 'default%s'|format(type) if getter.name == '_unnamed_' else getter.name %}
    {% set use_nullable = getter.return.kind in non_nullable_type_kinds %}
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
    {% elif getter.return.kind in pointer_type_kinds %}
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
    {% set exception = igetter.raises_exception if igetter else ngetter.raises_exception %}
    {{name}}* self = ({{name}}*)jsSelf->extraData();
    STARFISH_ASSERT(self->is{{name}}());
    {% call raises_exception(exception, 4) %}
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
    {% endcall %}
{% else %}
    // No getter found in {{ name }}
{% endif %}
    // NOTE Ignore this when there are multiple return statements in same depth
    return ExposableObjectGetOwnPropertyCallbackResult();
}

static bool {{ name }}DefineOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* jsSelf, ValueRef* key, ValueRef* value)
{
{% if isetter or nsetter %}
    {% set names = {'name': name, 'fname': 'Setter', 'aname': 'value', 'vname': 'valueTo'} %}
    {% set setarg = isetter.arguments[1] if isetter else nsetter.arguments[1] %}
    {% set isetter_name = 'defaultIndexedSetter' if isetter and isetter.name == '_unnamed_' else (isetter.name if isetter else '') %}
    {% set nsetter_name = 'defaultNamedSetter' if nsetter and nsetter.name == '_unnamed_' else (nsetter.name if nsetter else '') %}
    {% set iassing_exp = 'self->%s(idx, valueTo);'|format(isetter_name) %}
    {% set nassing_exp = 'self->%s(toBrowserString(state, key), valueTo);'|format(nsetter_name) %}
    {% set exception = isetter.raises_exception if isetter else nsetter.raises_exception %}
    {{name}}* self = ({{name}}*)jsSelf->extraData();
    STARFISH_ASSERT(self->is{{name}}());
    {{ util_macro.handle_arg(setarg, names)|trim }}
    {% call raises_exception(exception, 4) %}
    {% if isetter and nsetter %}
    uint32_t idx = key->toArrayIndex(state);
    if (idx == ValueRef::InvalidArrayIndexValue) {
        {{ nassing_exp|indent(8) }}
    } else if (idx < self->length()) {
        {{ iassing_exp|indent(8) }}
    }
    {% elif isetter %}
    uint32_t idx = key->toArrayIndex(state);
    if (idx != ValueRef::InvalidArrayIndexValue) {
        {{ iassing_exp|indent(8) }}
    }
    {% else %}
    {{ nassing_exp|indent(4) }}
    {% endif %}
    {% endcall %}
{% else %}
    // No setter found in {{ name }}
{% endif %}
    return true;
}

static bool {{ name }}DeleteOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* jsSelf, ValueRef* key)
{
    return true;
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
    GCVector<String*> enums;
    self->defaultNamedEnumerator(enums);
    for (size_t i = 0; i < enums.size(); i++) {
        v.push_back(ExposableObjectEnumerationCallbackResult(
            ValueRef::create(toJSString(enums[i])), false, true, false));
    }
    {% endif %}
    return v;
}

