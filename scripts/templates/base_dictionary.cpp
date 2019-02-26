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

#include "StarfishConfig.h"
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
{{name}} to{{name}}FromValueRef(ExecutionStateRef* state, ValueRef* from)
{
    if (from->isUndefinedOrNull()) {
        // Return empty dictionary
        return {{name}}();
    }
    if (!from->isObject()) {
        auto msg = StringRef::fromASCII("Failed to generate {{name}} from non-object");
        state->throwException(ValueRef::create(TypeErrorObjectRef::create(state, msg)));
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
{% for key in members %}
    {% if not key.unimplemented %}
    ValueRef* arg{{loop.index - 1}} = from->asObject()->get(state, ValueRef::create(StringRef::fromASCII("{{ key.name }}")));
    {% endif %}
{% endfor %}
    {{name}} result;
{% for key in members -%}
    {% if not key.unimplemented %}
    {% set names = {'name': name, 'kname': key.name,
                    'aname': 'arg%d'|format(loop.index - 1), 'vname': 'value%d'|format(loop.index - 1)} %}
    {{ util_macro.handle_arg(key, names)|trim }}
    result.set{{key.name|first_word_capitalize}}({{'value%d'|format(loop.index - 1)}});
    {% endif %}
{% endfor %}
    return result;
}

ValueRef* toValueRefFrom{{name}}(ExecutionStateRef* state, {{name}}& from)
{
    ObjectRef* result = ObjectRef::create(state);
{% for key in members %}
    {% if not key.unimplemented %}
    {% set vname = 'value%d'|format(loop.index - 1) %}
    {% set use_nullable = key.type.kind in non_nullable_type_kinds and key.type.nullable %}
    {{ util_macro.gen_declare_return_value(key.type, vname)|trim }}
    {{ vname }} = from.{{ key.name }}();
    {% if use_nullable %}
    if (!{{ vname }}.hasValue()) {
        result->set(state, ValueRef::create(StringRef::fromASCII("{{ key.name }}")), ValueRef::createNull());
    } else {
        result->set(state, ValueRef::create(StringRef::fromASCII("{{ key.name }}")), {{ util_macro.gen_native_to_jsvalue(key.type, '%s.getValue()'|format(vname)) }});
    }
    {% elif key.type.nullable %}
    if ({{ vname }} == nullptr) {
        result->set(state, ValueRef::create(StringRef::fromASCII("{{ key.name }}")), ValueRef::createNull());
    } else {
        result->set(state, ValueRef::create(StringRef::fromASCII("{{ key.name }}")), {{ vname }}->scriptValue());
    }
    {% elif key.type.kind in pointer_type_kinds %}
    STARFISH_ASSERT({{ vname }} != nullptr);
    result->set(state, ValueRef::create(StringRef::fromASCII("{{ key.name }}")), {{ util_macro.gen_native_to_jsvalue(key.type, vname) }});
    {% elif key.type.kind == 'Sequence' %}
    ArrayObjectRef* {{ key.name }}ArrayObj = ArrayObjectRef::create(state);
    for (unsigned idx = 0; idx < {{ vname }}.size(); idx++) {
        {% if key.type.data.kind in non_nullable_type_kinds and key.type.data.nullable %}
        ValueRef* item = {{ vname }}[idx].hasValue() ? {{ util_macro.gen_native_to_jsvalue(key.type.data, '%s[idx].getValue()'|format(vname)) }} : ValueRef::createNull();
        {% elif key.type.data.kind in pointer_type_kinds and key.type.data.nullable %}
        ValueRef* item = {{ vname }}[idx] != nullptr ? {{ util_macro.gen_native_to_jsvalue(key.type.data, '%s[idx]'|format(vname)) }} : ValueRef::createNull();
        {% else %}
            {% if key.type.data.kind in pointer_type_kinds %}
        STARFISH_ASSERT({{ vname }}[idx] != nullptr);
            {% endif %}
        ValueRef* item = {{ util_macro.gen_native_to_jsvalue(key.type.data, '%s[idx]'|format(vname)) }};
        {% endif %}
        {{ key.name }}ArrayObj->set(state, ValueRef::create(idx), item);
    }
    result->set(state, ValueRef::create(StringRef::fromASCII("{{ key.name }}")), ValueRef::create({{ key.name }}ArrayObj));
    {% else %}
    result->set(state, ValueRef::create(StringRef::fromASCII("{{ key.name }}")), {{ util_macro.gen_native_to_jsvalue(key.type, vname) }});
    {% endif %}
    {% endif %}
{% endfor %}
    return ValueRef::create(result);
}
}
{%- endcall %}
