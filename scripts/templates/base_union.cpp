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

#include "StarFishConfig.h"
#include "{{name|to_union_h_path}}"
#include "binding/ScriptWrappable.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

{% if used_unions %}
  {%- for union_item in used_unions %}
{% include 'union_impl.cpp' ignore missing %}
  {% endfor %}
{% endif %}

{{ name }} to{{ name }}FromValueRef(ExecutionStateRef* state, ValueRef* from)
{
    if (from->isUndefinedOrNull()) {
        return {{name}}();
    }
{% for subtype in data %}
    if ({{ util_macro.gen_check_type(subtype, 'from')|trim }}) {
    {% set use_nullable = subtype.kind in non_nullable_type_kinds and subtype.nullable %}
    {% set type_exp = util_macro.gen_type_str(subtype, use_nullable)|trim %}
    {% if subtype.kind == 'Sequence' %}
        {{ type_exp }} resultValue;
        {{ util_macro.get_arrayobject_to_native(subtype, 'from', 'resultValue')|indent(8) }}
        return {{ name }}::create{{ subtype.name }}(resultValue);
    {% elif use_nullable %}
        return {{ name }}::create{{ subtype.name }}(Nullable<{{ type_exp }}>({{ util_macro.gen_esvalue_to_native(subtype, 'from', False) }}));
    {% else %}
        return {{ name }}::create{{ subtype.name }}({{ util_macro.gen_esvalue_to_native(subtype, 'from', False) }});
    {% endif %}
    }
{% endfor %}

    THROW_EXCEPTION(ILLEGAL_INVOKE);
    return {{name}}();
}

ValueRef* toValueRefFrom{{ name }}(ExecutionStateRef* state, {{ name }}& from)
{
{% for subtype in data %}
    {% set use_nullable = subtype.kind in non_nullable_type_kinds and subtype.nullable %}
    if (from.is{{ subtype.name }}Value()) {
        {{ util_macro.gen_type_str(subtype, use_nullable) }} resultValue = from.get{{ subtype.name }}Value();
        {{ util_macro.gen_return_code(subtype, 'resultValue')|indent(8) }}
    }
{% endfor %}
    return ValueRef::createUndefined();
}

bool is{{ name }}(ExecutionStateRef* state, ValueRef* from)
{
{% for subtype in data %}
    if ({{ util_macro.gen_check_type(subtype, 'from')|trim }}) {
        return true;
    }
{% endfor %}
    return false;
}
}
