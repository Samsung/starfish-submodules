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

#include "StarFishConfig.h"
{% for item in include_paths %}
#include "{{item|to_h_path}}"
{% endfor %}
{% for item in used_unions %}
#include "{{item|to_union_h_path}}"
{% endfor %}
#include "{{file_path|to_h_path}}"

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

{% if used_dictionaries %}
  {%- for dictionary in used_dictionaries %}
{% include 'dictionary_impl.cpp' ignore missing %}
  {% endfor %}
{% endif %}
{% if used_unions %}
  {%- for union_item in used_unions %}
{% include 'union_impl.cpp' ignore missing %}
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
ExposableObjectGetOwnPropertyCallbackResult {{ name }}GetOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* self, ValueRef* key);
bool {{ name }}DefineOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* self, ValueRef* propertyName, ValueRef* value)
ExposableObjectEnumerationCallbackResultVector {{ name }}EnumerationCallback(ExecutionStateRef* state, ObjectRef* self);
bool {{ name }}DeleteOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* self, ValueRef* propertyName);
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
    ExecutionStateRef* state = ExecutionStateRef::create(context);
    {% if parent and parent.has_unforgeable %}
    attachUnforgeables{{ parent.name }}(instance, targetObject);
    {% endif %}
    {{ bind_common(condition_unforgeable_fn) }}
    state->destroy();
}
{% endif %}

FunctionObjectRef* binding{{ name }}(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ContextRef* context = scriptBindingInstance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(context);
{% include 'constructor_bind.cpp' ignore missing %}

    ObjectRef* targetObject = {{ name }}PrototypeObj;
    {{ bind_common(condition_binding_fn) }}
    state->destroy();
    return {{ name }}Function;
}
{% include 'constructor_named_bind.cpp' ignore missing %}

void {{ name }}::init(ScriptBindingInstance* instance, void* domObjectPointer)
{
    ContextRef* context = instance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(context);

    {% if primary_global %}
    m_object = context->globalObject();
    {% elif descriptor %}
    m_object = ObjectRef::createExposableObject(state, {{ name }}GetOwnPropertyCallback, {{ name }}DefineOwnPropertyCallback, {{ name }}EnumerationCallback, {{ name }}DeleteOwnPropertyCallback);
    {% else %}
    m_object = ObjectRef::create(state);
    {% endif %}
    m_object->setExtraData(domObjectPointer);
    m_object->giveInternalClassProperty("{{ name }}");

    scriptObject()->setPrototype(state, instance->fn{{ name }}()->getFunctionPrototype(state));
    ObjectRef* targetObject = scriptObject();

    {{ bind_common(condition_init_fn) }}
    {% if has_unforgeable %}
    attachUnforgeables{{ name }}(instance, targetObject);
    {% endif %}
    postInit(instance);
    state->destroy();
}

bool {{ name }}::is{{ name }}() const
{
    return true;
}

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
