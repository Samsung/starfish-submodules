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
{% if flags and flags|length > 0 %}
#if defined({{flags[0]}})
    {%- for idx in range(1, flags|length) %}
        {{-  ' && defined(%s)'|format(flags[idx]) -}}
    {% endfor %}
{% endif %}

{# FIXME: has to return boolean #}
{% macro can_include(in_prototype_chain, primary_global, unforgeable) -%}
  {% if unforgeable %}
    {{- 'true' if not in_prototype_chain else 'false' -}}
  {% else %}
    {% if primary_global %}
      {{- 'true' if not in_prototype_chain else 'false' -}}
    {% else %}
      {{- 'true' if in_prototype_chain else 'false' -}}
    {% endif %}
  {% endif %}
{%- endmacro %}

{% macro bind(in_prototype_chain, primary_global) -%}
{% if (not in_prototype_chain or not primary_global) %}
  {% if constants %}
    // Bind for constants
    {% for constant in constants %}
      {# I assume that constant does not appear on the primary global interface #}
      {% if not constant.unimplemented and in_prototype_chain %}
{% include 'constant_bind.cpp' ignore missing %}
      {% endif %}
    {% endfor %}
  {% endif %}
  {% if attributes %}
    // Bind for attributes
    {% for attribute in attributes %}
      {% if not attribute.unimplemented and (can_include(in_prototype_chain, primary_global, attribute.unforgeable) == 'true') %}
{% include 'attribute_bind.cpp' ignore missing %}
      {% endif %}
    {% endfor %}
  {% endif %}
  {% if functions %}
    // Bind for functions
    {% for function in functions %}
      {% if not function.unimplemented and (can_include(in_prototype_chain, primary_global, function.unforgeable) == 'true') %}
{% include 'function_bind.cpp' ignore missing %}
      {% endif %}
    {% endfor %}
  {% endif %}
{% endif %}
{%- endmacro -%}

#include "StarFishConfig.h"
{% for item in include_paths %}
#include "{{item|to_header_path}}"
{% endfor %}
#include "{{file_path|to_header_path}}"

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

{% if used_dictionaries %}
  {%- for dictionary in used_dictionaries %}
{% include 'dictionary_impl.cpp' ignore missing %}
  {% endfor %}
{% endif %}
{% if constructor and not constructor.unimplemented%}
// Implement for constructor
{% include 'constructor_impl.cpp' ignore missing %}
{% endif %}
{% if attributes %}
// Implement for attributes
  {% for attribute in attributes %}
    {% if not attribute.const and not attribute.unimplemented %}
{% include 'attribute_impl.cpp' ignore missing %}
    {% endif %}
  {% endfor %}
{% endif %}
{% if functions %}
// Implement for functions
  {% for function in functions %}
    {% if not function.unimplemented %}
      {% if function.kind in ['Operation', 'Stringifier'] %}
{% include 'function_impl.cpp' ignore missing %}
      {% elif function.kind == 'MultiOperation' %}
{% include 'function_multiform_impl.cpp' ignore missing %}
      {% endif %}
    {% endif %}
  {% endfor %}
{% endif %}

{%- if descriptor and not primary_global %}
  {% if descriptor.custom %}
ExposableObjectGetOwnPropertyCallbackResult {{ name }}GetOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* self, ValueRef* key);
void {{ name }}DefineOwnPropertyCallback(ExecutionStateRef* state, ObjectRef* self, ValueRef* propertyName, ValueRef* value)
ExposableObjectEnumerationCallbackResultVector {{ name }}EnumerationCallback(ExecutionStateRef* state, ObjectRef* self);
  {% else %}
    {% include 'descriptor_impl.cpp' ignore missing %}
  {% endif %}
{% endif %}

FunctionObjectRef* binding{{ name }}(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ContextRef* context = scriptBindingInstance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(context);
{% include 'constructor_bind.cpp' ignore missing %}
{{ bind(true, primary_global) }}
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
    m_object = ObjectRef::createExposableObject(state, {{ name }}GetOwnPropertyCallback, {{ name }}DefineOwnPropertyCallback, {{ name }}EnumerationCallback);
    {% else %}
    m_object = ObjectRef::create(state);
    {% endif %}
    m_object->setExtraData(domObjectPointer);
    m_object->giveInternalClassProperty("{{ name }}");

    scriptObject()->setPrototype(state, instance->fn{{ name }}()->getFunctionPrototype(state));
{{ bind(false, primary_global) }}
    postInit(instance);
    state->destroy();
}

bool {{ name }}::is{{ name }}() const
{
    return true;
}
}
{% if flags and flags|length > 0 %}
#endif
{% endif %}
