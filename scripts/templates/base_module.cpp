/**
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
{% for item in include_paths|sort %}
#include "{{item|to_h_path}}"
{% endfor %}
{% for item in used_unions|sort %}
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
  {%- for union_item in used_unions|sort %}
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

{% if iterable or maplike %}
{% if iterable|length == 1 %}
static ValueRef* entriesFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ObjectRef* obj = thisValue->toObject(state);
    ValueRef* fn = state->context()->globalObject()->arrayPrototype()->getOwnProperty(state, StringRef::createFromASCII("entries"));
    return fn->call(state, obj, 0, nullptr);
}

static ValueRef* keysFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ObjectRef* obj = thisValue->toObject(state);
    ValueRef* fn = state->context()->globalObject()->arrayPrototype()->getOwnProperty(state, StringRef::createFromASCII("keys"));
    return fn->call(state, obj, 0, nullptr);
}

static ValueRef* valuesFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ObjectRef* obj = thisValue->toObject(state);
    ValueRef* fn = state->context()->globalObject()->arrayPrototype()->getOwnProperty(state, StringRef::createFromASCII("values"));
    return fn->call(state, obj, 0, nullptr);
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

    ScriptBindingInstance* instance = fetchScriptBindingInstance(state->context());
    ObjectRef* iterator = thisValue->toObject(state)->get(state, state->context()->vmInstance()->iteratorSymbol())->call(state, thisValue, 0, nullptr)->toObject(state);
    ObjectRef* fn = arg->asObject();
    ValueRef** funcArgv = ALLOCA(sizeof(ValueRef*) * 3, ValueRef*);

    ValueRef* nextString = instance->stringNext();
    ValueRef* doneString = instance->stringDone();
    ValueRef* valueString = instance->stringValue();
    ValueRef* keyIndex = ValueRef::create(0);
    ValueRef* valIndex = ValueRef::create(1);

    size_t index = 0;
    while (true) {
        ObjectRef* result = iterator->get(state, nextString)->call(state, iterator, 0, nullptr)->toObject(state);
        if (result->get(state, doneString)->toBoolean(state)) {
            break;
        }
        funcArgv[0] = result->get(state, valueString);
        funcArgv[1] = ValueRef::create(index++);
        funcArgv[2] = originalObj->scriptValue();
        fn->call(state, receiver, 3, funcArgv);
    }

    return ValueRef::createUndefined();
}

{% else %}
{% set types = iterable if iterable else maplike.types %}
{% set keyType = util_macro.gen_type_str(types[0], True) %}
{% set valueType = util_macro.gen_type_str(types[1], True) %}
{% set valueTypeNonNullable = util_macro.gen_type_str(types[1], False) %}

static ObjectRef* createPrototype(ExecutionStateRef* state)
{
    ContextRef* context = state->context();
    ObjectRef* prototype = ObjectRef::create(state);
    prototype->defineDataProperty(
        state, context->vmInstance()->toStringTagSymbol(),
        StringRef::createFromASCII("{{ name }} Iterator"), false, false, true);
    prototype->setObjectPrototype(
        state, context->globalObject()->objectPrototype());
    return prototype;
}

static ValueRef* entriesFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ContextRef* context = state->context();
    IterationSource<Optional<String*>, {{ valueType }}>* iterationSource = originalObj->startIteration(state);
    GenericIteratorObjectRef* genericIter = GenericIteratorObjectRef::create(state, [](ExecutionStateRef* state, void* data) -> std::pair<ValueRef*, bool> {
            IterationSource<Optional<String*>, {{ valueType }}>* iterationSource = static_cast<IterationSource<Optional<String*>, {{ valueType }}>*> (data);
            Optional<String*> key;
            {{ valueType }} value;
            if (iterationSource->next(state, key, value) && key.hasValue() && value.hasValue()) {
                ArrayObjectRef* arrayObj = ArrayObjectRef::create(state);
                // Set key
                arrayObj->set(state, ValueRef::create(0), ValueRef::create(toJSString(key.value())));

                // Set Value
                {% if valueType == 'Optional<String*>' %}
                arrayObj->set(state, ValueRef::create(1), ValueRef::create(toJSString(value.value())));
                {% elif valueType == 'Optional<ScriptValue>' %}
                arrayObj->set(state, ValueRef::create(1), value.value());
                {% else %}
                arrayObj->set(state, ValueRef::create(1), value.value()->scriptValue());
                {% endif %}
                return std::make_pair(arrayObj, false);
            }
            return std::make_pair(ValueRef::createUndefined(), true);
    }, iterationSource);

    return genericIter;
}

static ValueRef* keysFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ContextRef* context = state->context();
    IterationSource<Optional<String*>, {{ valueType }}>* iterationSource = originalObj->startIteration(state);
    GenericIteratorObjectRef* genericIter = GenericIteratorObjectRef::create(state, [](ExecutionStateRef* state, void* data) -> std::pair<ValueRef*, bool> {
            IterationSource<Optional<String*>, {{ valueType }}>* iterationSource = static_cast<IterationSource<Optional<String*>, {{ valueType }}>*> (data);
            Optional<String*> key;
            {{ valueType }} value;
            if (iterationSource->next(state, key, value) && key.hasValue()) {
                // Set key
                return std::make_pair(ValueRef::create(toJSString(key.value())), false);
            }
            return std::make_pair(ValueRef::createUndefined(), true);

    }, iterationSource);

    return genericIter;
}

static ValueRef* valuesFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression) {
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    ContextRef* context = state->context();
    IterationSource<Optional<String*>, {{ valueType }}>* iterationSource = originalObj->startIteration(state);
    GenericIteratorObjectRef* genericIter = GenericIteratorObjectRef::create(state, [](ExecutionStateRef* state, void* data) -> std::pair<ValueRef*, bool> {
            IterationSource<Optional<String*>, {{ valueType }}>* iterationSource = static_cast<IterationSource<Optional<String*>, {{ valueType }}>*> (data);
            Optional<String*> key;
            {{ valueType }} value;
            if (iterationSource->next(state, key, value) && value.hasValue()) {
                // Set Value
                {% if valueType == 'Optional<String*>' %}
                return std::make_pair(ValueRef::create(toJSString(value.value())), false);
                {% elif valueType == 'Optional<ScriptValue>' %}
                return std::make_pair(value.value(), false);
                {% else %}
                return std::make_pair(value.value()->scriptValue(), false);
                {% endif %}
            }
            return std::make_pair(ValueRef::createUndefined(), true);

    }, iterationSource);

    return genericIter;
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
    ValueRef** funcArgv = ALLOCA(sizeof(ValueRef*) * 3, ValueRef*);

    while (obj->next(state, k, v)) {
        if (!k.hasValue()) {
            funcArgv[1] = ValueRef::createNull();
        } else {
        {% if keyType == 'Optional<String*>' %}
            funcArgv[1] = createScriptString(k.value());
        {% else %}
            funcArgv[1] = k.value()->scriptValue();
        {% endif %}
        }
        if (!v.hasValue()) {
            funcArgv[0] = ValueRef::createNull();
        } else {
        {% if valueType == 'Optional<String*>' %}
            funcArgv[0] = createScriptString(v.value());
        {% elif valueType == 'Optional<ScriptValue>' %}
            funcArgv[0] = v.value();
        {% else %}
            funcArgv[0] = v.value()->scriptValue();
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

{% if maplike %}
static ValueRef* getFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    size_t argCount = argc;
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "get", "{{name}}", reason);
        THROW_EXCEPTION(msg);
    }

    ValueRef* arg0 = argv[0];
    String* value0 = String::emptyString;
    value0 = toBrowserString(state, arg0);

    {{ valueType }} result;

    result = originalObj->get(value0);

    if (!result.hasValue()) {
        return ValueRef::createNull();
    }

    {% if valueType == 'Optional<String*>' %}
    return createScriptString(result.value());
    {% elif valueType == 'Optional<ScriptValue>' %}
    return result.value();
    {% else %}
    return result.value()->scriptValue();
    {% endif %}
}

static ValueRef* hasFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    size_t argCount = argc;
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "has", "{{name}}", reason);
        THROW_EXCEPTION(msg);
    }

    ValueRef* arg0 = argv[0];
    String* value0 = String::emptyString;
    value0 = toBrowserString(state, arg0);

    bool result = false;
    result = originalObj->has(value0);

    return ValueRef::create(result);
}

{% if maplike.readonly == false %}
static ValueRef* setFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    size_t argCount = argc;
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "set", "{{name}}", reason);
        THROW_EXCEPTION(msg);
    }

    ValueRef* arg0 = argv[0];
    ValueRef* arg1 = argv[1];

    String* key = String::emptyString;
    key = toBrowserString(state, arg0);

    {{ valueTypeNonNullable }} value;
    {% if valueTypeNonNullable == 'String*' %}
    value = toBrowserString(state, arg1);
    {% elif valueTypeNonNullable == 'ScriptValue' %}
    value = arg1;
    {% else %}
    CHECK_TYPEOF(arg1, {{ valueTypeNonNullable }});
    value = {{ valueTypeNonNullable }}(arg1->asObject()->extraData());
    {% endif %}

    originalObj->set(key, value);

    return ValueRef::createUndefined();
}

static ValueRef* deleteFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE({{name}});
    size_t argCount = argc;
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "delete", "{{name}}", reason);
        THROW_EXCEPTION(msg);
    }
 
    ValueRef* arg0 = argv[0];
    String* value0 = String::emptyString;
    value0 = toBrowserString(state, arg0);

    bool result = false;
    result = originalObj->deleteItem(value0);

    return ValueRef::create(result);
}


static ValueRef* clearFunction(ExecutionStateRef* state, ValueRef* thisValue, size_t argc, ValueRef** argv, bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE({{name}});

    originalObj->clear();

    return ValueRef::createUndefined();
}

{% endif %}
void bindMaplike{{ name }}(ScriptBindingInstance* instance, ObjectRef* targetObject)
{
    ContextRef* context = instance->scriptContext();
    Evaluator::execute(context, [](ExecutionStateRef* state, ScriptBindingInstance* instance, ObjectRef* targetObject) -> ValueRef* {
        ContextRef* context = instance->scriptContext();
        FunctionObjectRef* getFn = FunctionObjectRef::create(state,
            FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "get"), getFunction, 1, true, false));
        FunctionObjectRef* hasFn = FunctionObjectRef::create(state,
            FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "has"), hasFunction, 1, true, false));
        {% if maplike.readonly == False %}
        FunctionObjectRef* setFn = FunctionObjectRef::create(state,
            FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "set"), setFunction, 2, true, false));
        FunctionObjectRef* deleteFn = FunctionObjectRef::create(state,
            FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "delete"), deleteFunction, 1, true, false));
        FunctionObjectRef* clearFn = FunctionObjectRef::create(state,
            FunctionObjectRef::NativeFunctionInfo(AtomicStringRef::create(context, "clear"), clearFunction, 0, true, false));
        {% endif %}
        targetObject->defineDataProperty(state,
                ValueRef::create(StringRef::createFromASCII("get")),
                ValueRef::create(getFn),
                true, true, true);
        targetObject->defineDataProperty(state,
                ValueRef::create(StringRef::createFromASCII("has")),
                ValueRef::create(hasFn),
                true, true, true);
        {% if maplike.readonly == False %}
        targetObject->defineDataProperty(state,
                ValueRef::create(StringRef::createFromASCII("set")),
                ValueRef::create(setFn),
                true, true, true);
        targetObject->defineDataProperty(state,
                ValueRef::create(StringRef::createFromASCII("delete")),
                ValueRef::create(deleteFn),
                true, true, true);
        targetObject->defineDataProperty(state,
                ValueRef::create(StringRef::createFromASCII("clear")),
                ValueRef::create(clearFn),
                true, true, true);
        {% endif %}
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

        targetObject->defineDataProperty(state, context->vmInstance()->toStringTagSymbol(),
                ctorInfo.m_name->string(), false, false, true);

        {% if has_unscopable %}
        bindUnscopables{{ name }}(scriptBindingInstance, targetObject);
        {% endif %}
        {% if iterable or maplike %}
        bindIterable{{ name }}(scriptBindingInstance, targetObject);
        {% endif %}
        {% if maplike %}
        bindMaplike{{ name }}(scriptBindingInstance, targetObject);
        {% endif %}
        {{ bind_common(condition_binding_fn) }}
        return {{ name }}Function;
    }, scriptBindingInstance).result->asFunctionObject();
}
{% include 'constructor_named_bind.cpp' ignore missing %}

{% if generate_init_and_is or not static_interface %}
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

        self->scriptObject()->setObjectPrototype(state, instance->fn{{ name }}()->getFunctionPrototype(state));
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
