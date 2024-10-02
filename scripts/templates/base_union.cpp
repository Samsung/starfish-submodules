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
#include "StarfishConfig.h"

{% for item in include_paths|sort %}
#include "{{item|to_h_path}}"
{% endfor %}

#include "{{name|to_union_h_path}}"
#include "binding/ScriptWrappable.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {
{% for subtype in data %}
    {% if subtype.kind == 'Dictionary' %}
    {%- call util_macro.ifdef_and_exposed(subtype.flags, subtype.exposed, args.exposed) %}
extern {{ subtype.name }} to{{ subtype.name }}FromValueRef(ExecutionStateRef* state, ValueRef* from);
extern ValueRef* toValueRefFrom{{ subtype.name }}(ExecutionStateRef* state, const {{ subtype.name }}& from);
    {%- endcall -%}
    {% endif %}
{% endfor %}


{% if used_unions %}
  {%- for union_item in used_unions|sort %}
{% include 'union_impl.cpp' ignore missing %}
  {% endfor %}
{% endif %}

{{ name }} to{{ name }}FromValueRef(ExecutionStateRef* state, ValueRef* from)
{
{% set glob = {} %}
{% for subtype in data %}
    {% if subtype.kind == 'StringType' %}
        {% set _ = glob.update({'use_string':true, 'type':subtype}) %}
    {% endif %}
{% endfor %}
{% if glob.use_string == true %}
    if (from->isUndefined()) {
        return {{ name }}::create{{ glob.type.name }}({{ util_macro.gen_esvalue_to_native(glob.type, 'ValueRef::createUndefined()', False) }});
    }
    if (from->isNull()) {
        return {{ name }}::create{{ glob.type.name }}({{ util_macro.gen_esvalue_to_native(glob.type, 'ValueRef::createNull()', False) }});
    }
{% else %}
    if (from->isUndefinedOrNull()) {
        return {{name}}();
    }
{% endif %}
{% for subtype in data %}
    {%- call util_macro.ifdef_and_exposed(subtype.flags, subtype.exposed, args.exposed) %}
    if ({{ util_macro.gen_check_type(subtype, 'from')|trim }}) {
        {% set use_nullable = util_macro.is_non_nullable_type(subtype.kind) and subtype.nullable %}
        {% set type_exp = util_macro.gen_type_str(subtype, use_nullable)|trim %}
        {% if subtype.kind.startswith('SequenceOf') %}
            if (from->asObject()->getPrototype(state)->isObject() && ((from->asObject()->getPrototype(state)->asObject()->isArrayPrototypeObject())||from->asObject()->getPrototype(state)->asObject()->isTypedArrayPrototypeObject())) {
                {{ type_exp }} resultValue;
                {{ util_macro.get_arrayobject_to_native(subtype, 'from', 'resultValue')|indent(16) }}
                return {{ name }}::create{{ subtype.name }}(resultValue);
            {% if glob.use_string == true %}
            } else{
                // Fallback
                return {{ name }}::create{{ glob.type.name }}({{ util_macro.gen_esvalue_to_native(glob.type, 'from', False) }});
            }
            {% else %}
            }
            {% endif %}
        {% elif use_nullable %}
        return {{ name }}::create{{ subtype.name }}(Optional<{{ type_exp }}>({{ util_macro.gen_esvalue_to_native(subtype, 'from', False) }}));
        {% else %}
        return {{ name }}::create{{- util_macro.getTrimmedName(subtype) -}}({{ util_macro.gen_esvalue_to_native(subtype, 'from', False) }});
        {% endif %}
    }
    {%- endcall -%}
{% endfor %}

    return {{name}}();
}

ValueRef* toValueRefFrom{{ name }}(ExecutionStateRef* state, const {{ name }}& from)
{
{% for subtype in data %}
    {%- call util_macro.ifdef_and_exposed(subtype.flags, subtype.exposed, args.exposed) %}
        {% set use_nullable = util_macro.is_non_nullable_type(subtype.kind) and subtype.nullable %}
    if (from.is{{- util_macro.getTrimmedName(subtype) -}}Value()) {
        {{ util_macro.gen_type_str(subtype, use_nullable) }} resultValue = from.get{{- util_macro.getTrimmedName(subtype) -}}Value();
        {{ util_macro.gen_return_code(subtype, 'resultValue')|indent(8) }}
    }
    {%- endcall -%}
{% endfor %}
    return ValueRef::createUndefined();
}

bool is{{ name }}(ExecutionStateRef* state, ValueRef* from)
{
{% for subtype in data %}
    {%- call util_macro.ifdef_and_exposed(subtype.flags, subtype.exposed, args.exposed) %}
    if ({{ util_macro.gen_check_type(subtype, 'from')|trim }}) {
        return true;
    }
    {%- endcall -%}
{% endfor %}
    return false;
}
}

{% endcall %}
