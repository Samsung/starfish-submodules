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
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}'|format(caller())|indent(indent, true) }}
{% else %}
{{ caller() }}
{% endif %}
{%- endmacro -%}

{%- macro handle_getter(getter, type) %}

    {% set getter_name = 'default%s'|format(type) if getter.name == '_unnamed_' else getter.name %}
    {% set use_nullable =  util_macro.is_non_nullable_type(getter.return.kind) %}
    {% set result_name = 'result.getValue()' if use_nullable else 'result' %}
    {% set return_stm = 'return ExposableObjectGetOwnPropertyCallbackResult(%s, true, false, false);'|format(util_macro.gen_native_to_jsvalue(getter.return, result_name)) %}
    // {{type}}
    {% if type == 'NamedGetter' %}
    // Search matched property through prototype chain
    OptionalRef<ObjectRef> ref = scriptObject->getPrototypeObject(state);
    while (ref) {
        if (ref.value()->hasOwnProperty(state, key)) {
            return ExposableObjectGetOwnPropertyCallbackResult();
        }
        ref = ref.value()->getPrototypeObject(state);
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
    {% set exception = true if igetter and igetter.raises_exception else ngetter and ngetter.raises_exception %}
    STARFISH_ASSERT(((ScriptWrappable*)jsSelf->extraData())->is{{name}}());
    {{name}}* self = ({{name}}*)jsSelf->extraData();
    ObjectRef* scriptObject = jsSelf;
    {% call raises_exception(exception, 4) %}
    {% if igetter and ngetter %}
    uint32_t idx = key->toIndex32(state);
    if (idx == ValueRef::InvalidIndex32Value) {
        {{- handle_getter(ngetter, 'NamedGetter')|indent(4, False) }}
    } else if (idx < self->length()) {
        {{- handle_getter(igetter, 'IndexedGetter')|indent(4, False) }}
    }
    {% elif igetter %}
    uint32_t idx = key->toIndex32(state);
    if (idx != ValueRef::InvalidIndex32Value && idx < self->length()) {
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
    {% set exception = true if isetter and isetter.raises_exception else nsetter and nsetter.raises_exception %}
    STARFISH_ASSERT(((ScriptWrappable*)jsSelf->extraData())->is{{name}}());
    {{name}}* self = ({{name}}*)jsSelf->extraData();
    bool result = false;
    {% call raises_exception(exception, 4) %}
    {% if isetter and nsetter %}
    {{ util_macro.handle_arg(setarg, names)|trim }}
    uint32_t idx = key->toIndex32(state);
    if (idx == ValueRef::InvalidIndex32Value) {
        {{ 'result = %s'|format(nassing_exp)|indent(8) }}
    } else if (idx < self->length()) {
        {{ 'result = %s'|format(iassing_exp)|indent(8) }}
    }
    {% elif isetter %}
    uint32_t idx = key->toIndex32(state);
    if (idx != ValueRef::InvalidIndex32Value) {
        {{ util_macro.handle_arg(setarg, names)|trim|indent(4) }}
        {{ 'result = %s'|format(iassing_exp)|indent(8) }}
    }
    {% else %}
    {{ util_macro.handle_arg(setarg, names)|trim }}
    {{ 'result = %s'|format(nassing_exp)|indent(4) }}
    {% endif %}
    {% endcall %}
    return result;
{% else %}
    // No setter found in {{ name }}
    return false;
{% endif %}
}

static bool {{ name }}DeleteOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* jsSelf, ValueRef* key)
{
{% if ideleter or ndeleter %}
    {% set ideleter_name = 'defaultIndexedDeleter' if ideleter and ideleter.name == '_unnamed_' else (ideleter.name if ideleter else '') %}
    {% set ndeleter_name = 'defaultNamedDeleter' if ndeleter and ndeleter.name == '_unnamed_' else (ndeleter.name if ndeleter else '') %}
    {% set exception = true if ideleter and ideleter.raises_exception else ndeleter and ndeleter.raises_exception %}
    STARFISH_ASSERT(((ScriptWrappable*)jsSelf->extraData())->is{{name}}());
    {{name}}* self = ({{name}}*)jsSelf->extraData();
    bool result = false;
    {% call raises_exception(exception, 4) %}
    {% if ideleter and ndeleter %}
    uint32_t idx = key->toIndex32(state);
    if (idx == ValueRef::InvalidIndex32Value) {
        {{ 'result = self->%s(toBrowserString(state, key));'|format(ndeleter_name)|indent(4, False) }}
    } else if (idx < self->length()) {
        {{ 'result = self->%s(idx);'|format(ideleter_name)|indent(4, False) }}
    }
    {% elif ideleter %}
    uint32_t idx = key->toIndex32(state);
    if (idx != ValueRef::InvalidIndex32Value && idx < self->length()) {
        {{ 'result = self->%s(idx);'|format(ideleter_name)|indent(4, False) }}
    }
    {% elif ndeleter %}
    {{ 'result = self->%s(toBrowserString(state, key));'|format(ndeleter_name) }}
    {% endif %}
    {% endcall %}
    return result;
{% else %}
    return true;
{% endif %}
}

static ExposableObjectEnumerationCallbackResultVector {{ name }}EnumerationCallback(ExecutionStateRef* state, ObjectRef* jsSelf)
{
    STARFISH_ASSERT(((ScriptWrappable*)jsSelf->extraData())->is{{name}}());
    {{name}}* self = ({{name}}*)jsSelf->extraData();
    {% if igetter and igetter.enumerable and ngetter and ngetter.enumerable%}
    size_t len = self->length();
    GCVector<String*> enums;
    self->defaultNamedEnumerator(enums);

    ExposableObjectEnumerationCallbackResultVector v(self->length() + enums.size());

    for (size_t i = 0; i < len; i++) {
        v[i] = (ExposableObjectEnumerationCallbackResult(
            ValueRef::create(i), false, true, false));
    }

    for (size_t i = 0; i < enums.size(); i++) {
        v[i + len] = (ExposableObjectEnumerationCallbackResult(
            ValueRef::create(toJSString(enums[i])), false, true, false));
    }

    {% elif igetter and igetter.enumerable %}
    ExposableObjectEnumerationCallbackResultVector v(self->length());
    size_t len = self->length();
    for (size_t i = 0; i < len; i++) {
        v[i] = (ExposableObjectEnumerationCallbackResult(
            ValueRef::create(i), false, true, false));
    }
    {% elif ngetter and ngetter.enumerable %}
    GCVector<String*> enums;
    self->defaultNamedEnumerator(enums);
    ExposableObjectEnumerationCallbackResultVector v(enums.size());
    for (size_t i = 0; i < enums.size(); i++) {
        v[i] = (ExposableObjectEnumerationCallbackResult(
            ValueRef::create(toJSString(enums[i])), false, true, false));
    }
    {% else %}
    ExposableObjectEnumerationCallbackResultVector v;
    {% endif %}
    return v;
}

