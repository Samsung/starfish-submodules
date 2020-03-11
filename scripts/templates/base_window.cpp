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

{% macro bind_common_proxy(condition_macro) -%}
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
{% include 'function_bind_proxy.cpp' ignore missing %}
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

{%- if descriptor %}
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
        {% if parent and parent.has_unforgeable %}
        attachUnforgeables{{ parent.name }}(instance, targetObject);
        {% endif %}
        {{ bind_common(condition_unforgeable_fn) }}
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
    {% include 'constructor_bind.cpp' ignore missing %}

        ObjectRef* targetObject = {{ name }}PrototypeObj;
        {{ bind_common(condition_binding_fn) }}
        return {{ name }}Function;
    }, scriptBindingInstance).result.asFunctionObject();
}

void Window::init(ScriptBindingInstance* instance, void* domObjectPointer)
{
    ContextRef* context = instance->scriptContext();
    Evaluator::execute(context, [](ExecutionStateRef* state, ScriptBindingInstance* instance, void* domObjectPointer) -> ValueRef* {

        m_object = context->globalObject();
        m_object->setExtraData(domObjectPointer);
        m_object->defineDataProperty(state, context->vmInstance()->toStringTagSymbol(),
                StringRef::createFromASCII("Window"), false, false, true);

        scriptObject()->setPrototype(state, instance->fn{{ name }}()->getFunctionPrototype(state));
        ObjectRef* targetObject = scriptObject();

        {{ bind_common(condition_init_fn) }}
        {% if has_unforgeable %}
        attachUnforgeables{{ name }}(instance, targetObject);
        {% endif %}
        postInit(instance);
        return ValueRef::createUndefined();
    }, instance, domObjectPointer);
}

void WindowProxy::init(ScriptBindingInstance* instance, void* domObjectPointer)
{
    ContextRef* context = instance->scriptContext();
    Evaluator::execute(context, [](ExecutionStateRef* state, ScriptBindingInstance* instance, void* domObjectPointer) -> ValueRef* {

        {% if descriptor %}
        m_object = ObjectRef::createExposableObject(state, WindowGetOwnPropertyCallback, WindowDefineOwnPropertyCallback, WindowEnumerationCallback, WindowDeleteOwnPropertyCallback);
        {% else %}
        m_object = ObjectRef::create(state);
        {% endif %}
        m_object->setExtraData(domObjectPointer);
        m_object->giveInternalClassProperty("Window");

        scriptObject()->setPrototype(state, instance->fnWindow()->getFunctionPrototype(state));
        ObjectRef* windowObject = window()->scriptObject();
        ObjectRef* targetObject = scriptObject();

        {{ bind_common_proxy(condition_init_fn) }}
        {% if has_unforgeable %}
        attachUnforgeablesWindow(instance, targetObject);
        {% endif %}
        postInit(instance);
        return ValueRef::createUndefined();
    }, instance, domObjectPointer);
}

bool Window::isWindow() const
{
    return true;
}

bool WindowProxy::isWindowProxy() const
{
    return true;
}
}
{% endcall %}
