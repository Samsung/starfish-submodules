/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "ScriptBindingInstance.h"

#include "Binding.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

{% if used_dictionary %}
  {%- for dictionary in used_dictionary %}
    {% if not dictionary.unimplemented %}
{% include 'dictionary_impl.cpp' ignore missing %}
    {% endif %}
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
      {% if function.kind == 'Operation' %}
{% include 'function_impl.cpp' ignore missing %}
      {% elif function.kind == 'MultiOperation' %}
{% include 'function_multiform_impl.cpp' ignore missing %}
      {% endif %}
    {% endif %}
  {% endfor %}
{% endif %}
ESFunctionObject* binding{{ name }}(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
{% include 'constructor_bind.cpp' ignore missing %}

{% if constants %}
    // Bind for constants
  {% for constant in constants %}
    {% if not constant.unimplemented %}
{% include 'constant_bind.cpp' ignore missing %}
    {% endif %}
  {% endfor %}
{% endif %}
{% if attributes %}
    // Bind for attributes
  {% for attribute in attributes %}
    {% if not attribute.unimplemented %}
{% include 'attribute_bind.cpp' ignore missing %}
    {% endif %}
  {% endfor %}
{% endif %}
{% if functions %}
    // Bind for functions
  {% for function in functions %}
    {% if not function.unimplemented %}
{% include 'function_bind.cpp' ignore missing %}
    {% endif %}
  {% endfor %}
{% endif %}
    return {{ name }}Function;
}
{% include 'constructor_named_bind.cpp' ignore missing %}
}
{% if flags and flags|length > 0 %}
#endif
{% endif %}