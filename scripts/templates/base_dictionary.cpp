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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"
{% for item in include_paths %}
#include "{{item|to_header_path}}"
{% endfor %}
#include "{{file_path|to_header_path}}"

namespace StarFish {

using namespace escargot;
{% import 'util.cpp' as util_macro %}

{% if used_dictionaries %}
  {%- for dictionary in used_dictionaries %}
{% include 'dictionary_impl.cpp' ignore missing %}
  {% endfor %}
{% endif %}
{{name}} to{{name}}FromESValue(ESVMInstance* instance, ESValue& from)
{
    if (!from.isObject()) {
        auto msg = ESString::create("Failed to generate {{name}} from non-object");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
{% for key in members %}
    ESValue arg{{loop.index - 1}} = from.asESPointer()->asESObject()->get(ESString::create("{{ key.name }}"));
{% endfor %}
    {{name}} result;
{% for key in members -%}
    {{ util_macro.handle_arg(key, 'arg%d'|format(loop.index - 1), 'value%d'|format(loop.index - 1)) }}
    result.set{{key.name|first_word_capitalize}}({{'value%d'|format(loop.index - 1)}});
{% endfor %}
    return result;
}

ESValue toESValueFrom{{name}}(ESVMInstance* instance, {{name}}& from)
{
    ESObject* result = ESObject::create();
{% for key in members %}
    {% set vname = 'value%d'|format(loop.index - 1) %}
    {% set use_nullable = key.type.kind in nullable_kinds and key.type.nullable %}
    {{ util_macro.gen_declare_return_value(key.type, vname)|trim }}
    {{ vname }} = from.{{ key.name }}();
    {% if use_nullable %}
    if (!{{ vname }}.hasValue()) {
        result->set(ESString::create("{{ key.name }}"), ESValue(ESValue::ESNull));
    } else {
        result->set(ESString::create("{{ key.name }}"), {{ util_macro.gen_native_to_esvalue(key.type, '%s->getValue()'|format(vname)) }});
    }
    {% elif key.type.nullable %}
    if ({{ vname }} == nullptr) {
        result->set(ESString::create("{{ key.name }}"), ESValue(ESValue::ESNull));
    } else {
        result->set(ESString::create("{{ key.name }}"), {{ vname }}->scriptValue());
    }
    {% elif key.type.kind in pointer_kinds %}
    STARFISH_ASSERT({{ vname }} != nullptr);
    result->set(ESString::create("{{ key.name }}"), {{ util_macro.gen_native_to_esvalue(key.type, vname) }});
    {% else %}
    result->set(ESString::create("{{ key.name }}"), {{ util_macro.gen_native_to_esvalue(key.type, vname) }});
    {% endif %}
{% endfor %}
    return ESValue(result);
}
}
{% if flags and flags|length > 0 %}
#endif
{% endif %}

