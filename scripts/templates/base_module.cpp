/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
{% import 'util.cpp' as util_macro %}
{%- call util_macro.ifdef(flags) %}

{# FIXME: has to return boolean #}
{% macro bind_common(condition_macro) -%}
    // Bind for constants
  {% for constant in constants %}
    {% if condition_macro(constant)|trim == 'true' %}
{% include 'constant_bind.cpp' ignore missing %}
    {% endif %}
  {% endfor %}
    // Bind for attributes
  {% for attribute in attributes %}
    {% if condition_macro(attribute)|trim == 'true' %}
{% include 'attribute_bind.cpp' ignore missing %}
    {% endif %}
  {% endfor %}
    // Bind for functions
  {% for function in functions %}
    {% if condition_macro(function)|trim == 'true' %}
{% include 'function_bind.cpp' ignore missing %}
    {% endif %}
  {% endfor %}
{%- endmacro -%}

{% macro condition_unforgeable_fn(ir) -%}
    {% if ir.unforgeable %}
        true
    {% else %}
        false
    {% endif %}
{%- endmacro -%}

{% macro condition_init_fn(ir) -%}
    {% if primary_global and not ir.unforgeable %}
        true
    {% else %}
        false
    {% endif %}
{%- endmacro -%}

{% macro condition_binding_fn(ir) -%}
    {% if not primary_global and not ir.unforgeable %}
        true
    {% else %}
        false
    {% endif %}
{%- endmacro -%}

#include "StarfishConfig.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
{% for item in include_paths %}
#include "{{item|to_h_path}}"
{% endfor %}
{% for item in used_unions %}
#include "{{item|to_union_h_path}}"
{% endfor %}
#include "{{file_path|to_h_path}}"

#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {

{% if used_dictionaries %}
  {%- for dictionary in used_dictionaries %}
{% include 'dictionary_impl.cpp' ignore missing %}
  {% endfor %}
{% endif %}
{% if used_unions %}
  {%- for union_item in used_unions %}
{%- call util_macro.ifdef(used_unions_flags[union_item]) %}
{% include 'union_impl.cpp' ignore missing %}
{% endcall %}
  {% endfor %}
{% endif %}
{% if constructor %}
// Implement for constructor
{% include 'constructor_impl.cpp' ignore missing %}
{% endif %}
{% if constants %}
  {% for constant in constants %}
{% include 'constant_impl.cpp' ignore missing %}
  {% endfor %}
{% endif %}
{% if attributes %}
// Implement for attributes
  {% for attribute in attributes %}
{% include 'attribute_impl.cpp' ignore missing %}
  {% endfor %}
{% endif %}
{% if functions %}
// Implement for functions
  {% for function in functions %}
    {% if function.kind in ['Operation', 'Stringifier'] %}
{% include 'function_impl.cpp' ignore missing %}
    {% elif function.kind == 'MultiOperation' %}
{% include 'function_multiform_impl.cpp' ignore missing %}
    {% endif %}
  {% endfor %}
{% endif %}

{%- if descriptor and not primary_global %}
  {% if descriptor.custom %}
extern ExposableObjectGetOwnPropertyCallbackResult {{ name }}GetOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* self, ValueRef* key);
extern bool {{ name }}DefineOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* self, ValueRef* propertyName, ValueRef* value);
extern ExposableObjectEnumerationCallbackResultVector {{ name }}EnumerationCallback(ExecutionStateRef* state, ObjectRef* self);
extern bool {{ name }}DeleteOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* self, ValueRef* propertyName);
  {% else %}
    {% include 'descriptor_impl.cpp' ignore missing %}
  {% endif %}
{% endif %}

{% if has_unforgeable %}
    {% if parent and parent.has_unforgeable %}
extern void attachUnforgeables{{ parent.name }}(ScriptBindingInstance* instance, ObjectRef* targetObject);
    {% endif %}
void attachUnforgeables{{ name }}(ScriptBindingInstance* instance, ObjectRef* targetObject)
{
    ContextRef* context = instance->scriptContext();
    Evaluator::execute(context, [](ExecutionStateRef* state, ScriptBindingInstance* instance, ObjectRef* targetObject) -> ValueRef* {
        ContextRef* context = instance->scriptContext();
        {% if parent and parent.has_unforgeable %}
        attachUnforgeables{{ parent.name }}(instance, targetObject);
        {% endif %}
        {{ bind_common(condition_unforgeable_fn) }}
        return ValueRef::createUndefined();
    }, instance, targetObject);
}
{% endif %}

{% if has_unscopable %}
void bindUnscopables{{ name }}(ScriptBindingInstance* instance, ObjectRef* targetObject)
{
    ContextRef* context = instance->scriptContext();
    Evaluator::execute(context, [](ExecutionStateRef* state, ScriptBindingInstance* instance, ObjectRef* targetObject) -> ValueRef* {
        ContextRef* context = instance->scriptContext();
        ObjectRef* unscopableObject = ObjectRef::create(state);
        {% for attribute in attributes %}
            {% if attribute.unscopable %}
        unscopableObject->defineDataProperty(state, StringRef::createFromASCII("{{ attribute.name }}"),
                ValueRef::create(true), true, true, true);
            {% endif %}
        {% endfor %}
        {% for function in functions %}
            {% if function.unscopable %}
        unscopableObject->defineDataProperty(state, StringRef::createFromASCII("{{ function.name }}"),
                ValueRef::create(true), true, true, true);
            {% endif %}
        {% endfor %}
        targetObject->defineDataProperty(state, context->vmInstance()->unscopablesSymbol(),
                unscopableObject, false, false, true);
        return ValueRef::createUndefined();
    }, instance, targetObject);
}
{% endif %}

{% if iterable %}
{% if iterable|length == 1 %}
static ValueRef* entriesFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ObjectRef* result = ((ArrayObjectRef*)thisValue->toObject(state))->entries(state);
    if (result == nullptr) {
        return ValueRef::createNull();
    }
    return result;
}

static ValueRef* keysFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ObjectRef* result = ((ArrayObjectRef*)thisValue->toObject(state))->keys(state);
    if (result == nullptr) {
        return ValueRef::createNull();
    }
    return result;
}

static ValueRef* valuesFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ObjectRef* result = ((ArrayObjectRef*)thisValue->toObject(state))->values(state);
    if (result == nullptr) {
        return ValueRef::createNull();
    }
    return result;
}

static ValueRef* forEachFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});

    ValueRef* receiver = thisValue;
    if (argc == 0) {
        auto msg = StringRef::createFromASCII("Failed to execute 'forEach' on '{{name}}'");
        state->throwException(TypeErrorObjectRef::create(state, msg));
    }

    ValueRef* arg = argv[0];
    if (!arg->isCallable()) {
        auto msg = StringRef::createFromASCII("Failed to execute 'forEach' on '{{name}}'");
        state->throwException(TypeErrorObjectRef::create(state, msg));
    }
    if (argc == 2) {
        receiver = argv[1];
    }

    IteratorObjectRef* obj = ((ArrayObjectRef*)thisValue->toObject(state))->entries(state);
    ObjectRef* fn = (ObjectRef*)arg;

    ObjectRef* next = obj->next(state)->asObject();
    ValueRef* doneString = StringRef::createFromASCII("done");
    ValueRef* valueString = StringRef::createFromASCII("value");
    ValueRef* keyIndex = ValueRef::create(0);
    ValueRef* valIndex = ValueRef::create(1);
    while (!next->get(state, doneString)->toBoolean(state)) {
        ValueRef** funcArgv = ALLOCA(sizeof(ValueRef*) * 3, ValueRef*);
        ObjectRef* valRef = next->get(state, valueString)->asObject();
        funcArgv[0] = valRef->get(state, valIndex);
        funcArgv[1] = valRef->get(state, keyIndex);
        funcArgv[2] = originalObj->scriptValue();
        fn->call(state, receiver, 3, funcArgv);
        next = obj->next(state)->asObject();
    }
    return ValueRef::createUndefined();
}

{% else %}
{% set isStringTypeKey = (iterable[0].name == 'DOMString' or iterable[0].name == 'ByteString' or iterable[0].name == 'USVString') %}
{% set isStringTypeValue = (iterable[1].name == 'DOMString' or iterable[1].name == 'ByteString' or iterable[1].name == 'USVString') %}
{% set keyType = 'Nullable<String*>' if isStringTypeKey else 'Nullable<' + iterable[0].name + '*>' %}
{% set valueType = 'Nullable<String*>' if isStringTypeValue else 'Nullable<' + iterable[1].name + '*>' %}

static ObjectRef* createPrototype(ExecutionStateRef* state)
{
    ContextRef* context = state->context();
    ObjectRef* prototype = ObjectRef::create(state);
    prototype->defineDataProperty(
        state, context->vmInstance()->toStringTagSymbol(),
        StringRef::createFromASCII("{{ name }} Iterator"), false, false, true);
    prototype->setPrototype(
        state, context->globalObject()->objectPrototype());
    return prototype;
}

static ValueRef* nextEntries(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    IterationSource<{{ keyType }}, {{ valueType }}>* obj = (IterationSource<{{ keyType }}, {{ valueType }}>*)((ObjectRef*)thisValue)->extraData();
    STARFISH_ASSERT(obj);
    {{ keyType }} k;
    {{ valueType }} v;
    bool hasValue = obj->next(state, k, v);
    ObjectRef* ret = ObjectRef::create(state);
    ValueRef* value;
    if (hasValue) {
        ArrayObjectRef* arrayObj = ArrayObjectRef::create(state);
        if (!k.hasValue()) {
            arrayObj->set(state, ValueRef::create(0), ValueRef::createNull());
        } else {
        {% if isStringTypeKey %}
            arrayObj->set(state, ValueRef::create(0), ValueRef::create(createScriptString(k.getValue())));
        {% else %}
            arrayObj->set(state, ValueRef::create(0), k.getValue()->scriptValue());
        {% endif %}
        }
        if (!v.hasValue()) {
            arrayObj->set(state, ValueRef::create(1), ValueRef::createNull());
        } else {
        {% if isStringTypeValue %}
            arrayObj->set(state, ValueRef::create(1), ValueRef::create(createScriptString(v.getValue())));
        {% else %}
            arrayObj->set(state, ValueRef::create(1), v.getValue()->scriptValue());
        {% endif %}
        }
        value = arrayObj;
    } else {
        value = ValueRef::createUndefined();
    }

    ret->defineDataProperty(state,
        StringRef::createFromASCII("value"),
        value,
        true, true, true);
    ret->defineDataProperty(state,
        StringRef::createFromASCII("done"),
        ValueRef::create(!hasValue),
        true, true, true);
    return ret;
}

static ValueRef* entriesFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ContextRef* context = state->context();
    IteratorObjectRef* ret = IteratorObjectRef::create(state);
    ObjectRef* prototype = createPrototype(state);

    ret->setExtraData(originalObj->startIteration(state));
    ret->setPrototype(state, prototype);

    FunctionObjectRef* nextFn = FunctionObjectRef::create(state,
        FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "next"), nextEntries, 0, true, false));
    prototype->defineDataProperty(state,
            StringRef::createFromASCII("next"),
            nextFn,
            true, true, true);
    return ret;
}

static ValueRef* nextKeys(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    IterationSource<{{ keyType }}, {{ valueType }}>* obj = (IterationSource<{{ keyType }}, {{ valueType }}>*)((ObjectRef*)thisValue)->extraData();
    STARFISH_ASSERT(obj);
    {{ keyType }} k;
    {{ valueType }} v;
    bool hasValue = obj->next(state, k, v);
    ObjectRef* ret = ObjectRef::create(state);
    ValueRef* value;
    if (hasValue) {
        if (!k.hasValue()) {
            value = ValueRef::createNull();
        } else {
        {% if isStringTypeKey %}
            value = createScriptString(k.getValue());
        {% else %}
            value = k.getValue()->scriptValue();
        {% endif %}
        }
    } else {
        value = ValueRef::createUndefined();
    }

    ret->defineDataProperty(state,
        StringRef::createFromASCII("value"),
        value,
        true, true, true);
    ret->defineDataProperty(state,
        StringRef::createFromASCII("done"),
        ValueRef::create(!hasValue),
        true, true, true);
    return ret;
}

static ValueRef* keysFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ContextRef* context = state->context();
    IteratorObjectRef* ret = IteratorObjectRef::create(state);
    ObjectRef* prototype = createPrototype(state);
    ret->setExtraData(originalObj->startIteration(state));
    ret->setPrototype(state, prototype);

    FunctionObjectRef* nextFn = FunctionObjectRef::create(state,
        FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "next"), nextKeys, 0, true, false));

    prototype->defineDataProperty(state,
            StringRef::createFromASCII("next"),
            nextFn,
            true, true, true);
    return ret;
}

static ValueRef* nextValues(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    IterationSource<{{ keyType }}, {{ valueType }}>* obj = (IterationSource<{{ keyType }}, {{ valueType }}>*)((ObjectRef*)thisValue)->extraData();
    STARFISH_ASSERT(obj);
    {{ keyType }} k;
    {{ valueType }} v;
    bool hasValue = obj->next(state, k, v);
    ObjectRef* ret = ObjectRef::create(state);
    ValueRef* value;
    if (hasValue) {
        if (!v.hasValue()) {
            value = ValueRef::createNull();
        } else {
        {% if isStringTypeKey %}
            value = createScriptString(v.getValue());
        {% else %}
            value = v.getValue()->scriptValue();
        {% endif %}
        }
    } else {
        value = ValueRef::createUndefined();
    }

    ret->defineDataProperty(state,
        StringRef::createFromASCII("value"),
        value,
        true, true, true);
    ret->defineDataProperty(state,
        StringRef::createFromASCII("done"),
        ValueRef::create(!hasValue),
        true, true, true);
    return ret;
}

static ValueRef* valuesFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ContextRef* context = state->context();
    IteratorObjectRef* ret = IteratorObjectRef::create(state);
    ObjectRef* prototype = createPrototype(state);
    ret->setExtraData(originalObj->startIteration(state));
    ret->setPrototype(state, prototype);

    FunctionObjectRef* nextFn = FunctionObjectRef::create(state,
        FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "next"), nextValues, 0, true, false));

    prototype->defineDataProperty(state,
            StringRef::createFromASCII("next"),
            nextFn,
            true, true, true);
    return ret;
}

static ValueRef* forEachFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ValueRef* receiver = thisValue;
    if (argc == 0) {
        auto msg = StringRef::createFromASCII("Failed to execute 'forEach' on '{{name}}'");
        state->throwException(TypeErrorObjectRef::create(state, msg));
    }

    ValueRef* arg = argv[0];
    if (!arg->isCallable()) {
        auto msg = StringRef::createFromASCII("Failed to execute 'forEach' on '{{name}}'");
        state->throwException(TypeErrorObjectRef::create(state, msg));
    }
    if (argc == 2) {
        receiver = argv[1];
    }

    ObjectRef* fn = (ObjectRef*)arg;
    {{ keyType }} k;
    {{ valueType }} v;
    IterationSource<{{ keyType }}, {{ valueType }}>* obj = originalObj->startIteration(state);
    while (obj->next(state, k, v)) {
        ValueRef** funcArgv = ALLOCA(sizeof(ValueRef*) * 3, ValueRef*);
        if (!k.hasValue()) {
            funcArgv[1] = ValueRef::createNull();
        } else {
        {% if isStringTypeKey %}
            funcArgv[1] = createScriptString(k.getValue());
        {% else %}
            funcArgv[1] = k.getValue()->scriptValue();
        {% endif %}
        }
        if (!v.hasValue()) {
            funcArgv[0] = ValueRef::createNull();
        } else {
        {% if isStringTypeValue %}
            funcArgv[0] = createScriptString(v.getValue());
        {% else %}
            funcArgv[0] = v.getValue()->scriptValue();
        {% endif %}
        }

        funcArgv[2] = originalObj->scriptValue();
        fn->call(state, receiver, 3, funcArgv);
    }
    return ValueRef::createUndefined();
}
{% endif %}

void bindIterable{{ name }}(ScriptBindingInstance* instance, ObjectRef* targetObject)
{
    ContextRef* context = instance->scriptContext();
    Evaluator::execute(context, [](ExecutionStateRef* state, ScriptBindingInstance* instance, ObjectRef* targetObject) -> ValueRef* {
        ContextRef* context = instance->scriptContext();
        FunctionObjectRef* entriesFn = FunctionObjectRef::create(state,
            FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "entries"), entriesFunction, 0, true, false));
        FunctionObjectRef* keysFn = FunctionObjectRef::create(state,
            FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "keys"), keysFunction, 0, true, false));
        FunctionObjectRef* valuesFn = FunctionObjectRef::create(state,
            FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "values"), valuesFunction, 0, true, false));
        FunctionObjectRef* forEachFn = FunctionObjectRef::create(state,
            FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "forEach"), forEachFunction, 1, true, false));

        targetObject->defineDataProperty(state,
                ValueRef::create(StringRef::createFromASCII("entries")),
                ValueRef::create(entriesFn),
                true, true, true);
        targetObject->defineDataProperty(state,
                ValueRef::create(StringRef::createFromASCII("keys")),
                ValueRef::create(keysFn),
                true, true, true);
        targetObject->defineDataProperty(state,
                ValueRef::create(StringRef::createFromASCII("values")),
                ValueRef::create(valuesFn),
                true, true, true);
        targetObject->defineDataProperty(state,
                ValueRef::create(StringRef::createFromASCII("forEach")),
                ValueRef::create(forEachFn),
                true, true, true);
        {% set iteratorSymbolFunc = 'valuesFn' if iterable|length == 1 else 'entriesFn' %}
        targetObject->defineDataProperty(state,
                ValueRef::create(context->vmInstance()->iteratorSymbol()),
                ValueRef::create({{ iteratorSymbolFunc }}),
                true, true, true);

        return ValueRef::createUndefined();
    }, instance, targetObject);
}
{% endif %}

FunctionObjectRef* binding{{ name }}(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ContextRef* context = scriptBindingInstance->scriptContext();
    return Evaluator::execute(context, [](ExecutionStateRef* state, ScriptBindingInstance* scriptBindingInstance) -> ValueRef* {
        ContextRef* context = scriptBindingInstance->scriptContext();
        {% include 'constructor_bind.cpp' ignore missing %}

        ObjectRef* targetObject = {{ name }}PrototypeObj;
        {% if has_unscopable %}
        bindUnscopables{{ name }}(scriptBindingInstance, targetObject);
        {% endif %}
        {% if iterable %}
        bindIterable{{ name }}(scriptBindingInstance, targetObject);
        {% endif %}
        {{ bind_common(condition_binding_fn) }}
        return {{ name }}Function;
    }, scriptBindingInstance).result->asFunctionObject();
}
{% include 'constructor_named_bind.cpp' ignore missing %}

{% if not static_interface %}
void {{ name }}::init(ScriptBindingInstance* instance, void* domObjectPointer)
{
    ContextRef* context = instance->scriptContext();
    Evaluator::execute(context, [](ExecutionStateRef* state, ScriptBindingInstance* instance, void* domObjectPointer, {{ name }}* self) -> ValueRef* {
        ContextRef* context = instance->scriptContext();
        {% if primary_global %}
        self->m_object = context->globalObject();
        {% elif descriptor %}
        self->m_object = ObjectRef::createExposableObject(state, {{ name }}GetOwnPropertyCallback, {{ name }}DefineOwnPropertyCallback, {{ name }}EnumerationCallback, {{ name }}DeleteOwnPropertyCallback);
        {% else %}
        self->m_object = ObjectRef::create(state);
        {% endif %}
        self->m_object->setExtraData(domObjectPointer);
        self->m_object->defineDataProperty(state, context->vmInstance()->toStringTagSymbol(),
                StringRef::createFromASCII("{{ name }}"), false, false, true);

        self->scriptObject()->setPrototype(state, instance->fn{{ name }}()->getFunctionPrototype(state));
        ObjectRef* targetObject = self->scriptObject();

        {{ bind_common(condition_init_fn) }}
        {% if has_unforgeable %}
        attachUnforgeables{{ name }}(instance, targetObject);
        {% endif %}
        return ValueRef::createUndefined();
    }, instance, domObjectPointer, this);

    postInit(instance);
}

bool {{ name }}::is{{ name }}() const
{
    return true;
}
{% endif %}

{% if serializable %}
bool {{ name }}::isSerializable() const
{
    return true;
}

Serializable* {{ name }}::toSerializable() const
{
    return (Serializable*)this;
}
{% endif %}

{% if transferable %}
bool {{ name }}::isTransferable() const
{
    return true;
}

Transferable* {{ name }}::toTransferable() const
{
    return (Transferable*)this;
}
{% endif %}
}
{% endcall %}
