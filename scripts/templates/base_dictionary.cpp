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

{% if used_unions %}
  {%- for union_item in used_unions|sort %}
{% include 'union_impl.cpp' ignore missing %}
  {% endfor %}
{% endif %}
{% if used_dictionaries %}
  {%- for dictionary in used_dictionaries %}
{% include 'dictionary_impl.cpp' ignore missing %}
  {% endfor %}
{% endif %}
{{name}} to{{name}}FromValueRef(ExecutionStateRef* state, ValueRef* from)
{
    if (from->isUndefinedOrNull()) {
        // Return empty dictionary
        return {{name}}();
    }
    if (!from->isObject()) {
        auto msg = StringRef::createFromASCII("Failed to generate {{name}} from non-object");
        state->throwException(ValueRef::create(TypeErrorObjectRef::create(state, msg)));
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
{% for key in members %}
    {% if not key.unimplemented %}
    ValueRef* arg{{loop.index - 1}} = from->asObject()->get(state, ValueRef::create(StringRef::createFromASCII("{{ util_macro.gen_attr_name(key) }}")));
    {% endif %}
{% endfor %}
    {{name}} result;
{% for key in members -%}
    {% if not key.unimplemented %}
    {% set names = {'name': name, 'kname': util_macro.gen_attr_name(key),
                    'aname': 'arg%d'|format(loop.index - 1), 'vname': 'value%d'|format(loop.index - 1)} %}
    {{ util_macro.handle_arg(key, names)|trim }}
       {% if not key.default %}
    // Set value if arg is not Undefined, because it hasn't a default value.
    if (!{{ names.aname }}->isUndefined()) {
        result.set{{util_macro.gen_attr_name(key)|first_word_capitalize}}({{'value%d'|format(loop.index - 1)}});
    }
        {% else %}
    // Always set value because it has a default value.
    result.set{{util_macro.gen_attr_name(key)|first_word_capitalize}}({{'value%d'|format(loop.index - 1)}});
        {% endif %}
    {% endif %}
{% endfor %}
    return result;
}

ValueRef* toValueRefFrom{{name}}(ExecutionStateRef* state, const {{name}}& from)
{
    ObjectRef* result = ObjectRef::create(state);
{% for key in members %}
    {% if not key.unimplemented %}
    {% set vname = 'value%d'|format(loop.index - 1) %}
    {% set use_nullable = util_macro.is_non_nullable_type(key.type.kind) and key.type.nullable %}
    {{ util_macro.gen_declare_return_value(key.type, vname)|trim }}
    {{ vname }} = from.{{util_macro.gen_attr_name(key)}}();

    {% if use_nullable %}
    if (!{{ vname }}.hasValue()) {
        result->set(state, ValueRef::create(StringRef::createFromASCII("{{ util_macro.gen_attr_name(key) }}")), ValueRef::createNull());
    } else {
        {% if key.type.kind.startswith('SequenceOf') %}
        ArrayObjectRef* {{ util_macro.gen_attr_name(key) }}ArrayObj = ArrayObjectRef::create(state);
        for (unsigned idx = 0; idx < {{ vname }}.value().size(); idx++) {
            {% if util_macro.is_non_nullable_type(key.type.data.kind) and key.type.data.nullable %}
            ValueRef* item = {{ vname }}.value()[idx].hasValue() ? {{ util_macro.gen_native_to_jsvalue(key.type.data, '%s.value()[idx].getValue()'|format(vname)) }} : ValueRef::createNull();
            {% elif key.type.data.kind in pointer_type_kinds and key.type.data.nullable %}
            ValueRef* item = {{ vname }}.value()[idx] != nullptr ? {{ util_macro.gen_native_to_jsvalue(key.type.data, '%s.value()[idx]'|format(vname)) }} : ValueRef::createNull();
            {% else %}
                {% if key.type.data.kind in pointer_type_kinds %}
            STARFISH_ASSERT({{ vname }}.value()[idx] != nullptr);
                {% endif %}
            ValueRef* item = {{ util_macro.gen_native_to_jsvalue(key.type.data, '%s.value()[idx]'|format(vname)) }};
            {% endif %}
            {{ util_macro.gen_attr_name(key) }}ArrayObj->set(state, ValueRef::create(idx), item);
        }
        result->set(state, ValueRef::create(StringRef::createFromASCII("{{ util_macro.gen_attr_name(key) }}")), ValueRef::create({{ util_macro.gen_attr_name(key) }}ArrayObj));
        {% else %}
        result->set(state, ValueRef::create(StringRef::createFromASCII("{{ util_macro.gen_attr_name(key) }}")), {{ util_macro.gen_native_to_jsvalue(key.type, '%s.getValue()'|format(vname)) }});
        {% endif %}
    }
    {% elif key.type.nullable %}
    if ({{ vname }} == nullptr) {
        result->set(state, ValueRef::create(StringRef::createFromASCII("{{ util_macro.gen_attr_name(key) }}")), ValueRef::createNull());
    } else {
        result->set(state, ValueRef::create(StringRef::createFromASCII("{{ util_macro.gen_attr_name(key) }}")), {{ vname }}->scriptValue());
    }
    {% elif key.type.kind in pointer_type_kinds %}
    STARFISH_ASSERT({{ vname }} != nullptr);
    result->set(state, ValueRef::create(StringRef::createFromASCII("{{ util_macro.gen_attr_name(key) }}")), {{ util_macro.gen_native_to_jsvalue(key.type, vname) }});
    {% elif key.type.kind.startswith('SequenceOf') %}
    ArrayObjectRef* {{ util_macro.gen_attr_name(key) }}ArrayObj = ArrayObjectRef::create(state);
    for (unsigned idx = 0; idx < {{ vname }}.size(); idx++) {
        {% if util_macro.is_non_nullable_type(key.type.data.kind) and key.type.data.nullable %}
        ValueRef* item = {{ vname }}[idx].hasValue() ? {{ util_macro.gen_native_to_jsvalue(key.type.data, '%s[idx].getValue()'|format(vname)) }} : ValueRef::createNull();
        {% elif key.type.data.kind in pointer_type_kinds and key.type.data.nullable %}
        ValueRef* item = {{ vname }}[idx] != nullptr ? {{ util_macro.gen_native_to_jsvalue(key.type.data, '%s[idx]'|format(vname)) }} : ValueRef::createNull();
        {% else %}
            {% if key.type.data.kind in pointer_type_kinds %}
        STARFISH_ASSERT({{ vname }}[idx] != nullptr);
            {% endif %}
        ValueRef* item = {{ util_macro.gen_native_to_jsvalue(key.type.data, '%s[idx]'|format(vname)) }};
        {% endif %}
        {{ util_macro.gen_attr_name(key) }}ArrayObj->set(state, ValueRef::create(idx), item);
    }
    result->set(state, ValueRef::create(StringRef::createFromASCII("{{ util_macro.gen_attr_name(key) }}")), ValueRef::create({{ util_macro.gen_attr_name(key) }}ArrayObj));
    {% else %}
    result->set(state, ValueRef::create(StringRef::createFromASCII("{{ util_macro.gen_attr_name(key) }}")), {{ util_macro.gen_native_to_jsvalue(key.type, vname) }});
    {% endif %}
    {% endif %}
{% endfor %}
    return ValueRef::create(result);
}
}
{%- endcall %}
