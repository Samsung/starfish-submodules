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
#include "{{name|to_union_h_path}}"
#include "binding/ScriptWrappable.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {

{% if used_unions %}
  {%- for union_item in used_unions %}
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
        {% set use_nullable = subtype.kind in non_nullable_type_kinds and subtype.nullable %}
        {% set type_exp = util_macro.gen_type_str(subtype, use_nullable)|trim %}
        {% if subtype.kind.startswith('SequenceOf') %}
            if (from->asObject()->getPrototype(state)->isObject() && ((from->asObject()->getPrototype(state)->asObject()->isArrayPrototypeObject())||from->asObject()->getPrototype(state)->asObject()->isTypedArrayPrototypeObject())) {
                {{ type_exp }} resultValue;
                {{ util_macro.get_arrayobject_to_native(subtype, 'from', 'resultValue')|indent(16) }}
                return {{ name }}::create{{ subtype.name }}(resultValue);
            }else{
                return {{ name }}::createDOMString({{ util_macro.gen_esvalue_to_native({'kind': 'StringType', 'name': 'DOMString'}, 'from', False) }});
            }
        {% elif use_nullable %}
        return {{ name }}::create{{ subtype.name }}(Nullable<{{ type_exp }}>({{ util_macro.gen_esvalue_to_native(subtype, 'from', False) }}));
        {% else %}
        return {{ name }}::create{{ subtype.name }}({{ util_macro.gen_esvalue_to_native(subtype, 'from', False) }});
        {% endif %}
    }
    {%- endcall -%}
{% endfor %}

    THROW_EXCEPTION(ILLEGAL_INVOKE);
    return {{name}}();
}

ValueRef* toValueRefFrom{{ name }}(ExecutionStateRef* state, const {{ name }}& from)
{
{% for subtype in data %}
    {%- call util_macro.ifdef_and_exposed(subtype.flags, subtype.exposed, args.exposed) %}
        {% set use_nullable = subtype.kind in non_nullable_type_kinds and subtype.nullable %}
    if (from.is{{ subtype.name }}Value()) {
        {{ util_macro.gen_type_str(subtype, use_nullable) }} resultValue = from.get{{ subtype.name }}Value();
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
