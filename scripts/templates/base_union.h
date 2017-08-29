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
{%- macro gen_item_type_exp(item) -%}
{{ util_macro.gen_type_str(item, item.kind in non_nullable_type_kinds and item.nullable) }}
{%- endmacro -%}
#ifndef __StarFish{{ name }}__
#define __StarFish{{ name }}__

#include "binding/ScriptWrappable.h"
{% for item in include_paths %}
#include "{{item|to_h_path}}"
{% endfor %}
{% for item in used_unions %}
#include "{{item|to_union_h_path}}"
{% endfor %}

namespace StarFish {

class {{ name }} {
public:
    STARFISH_MAKE_STACK_ALLOCATED()

    enum ValueKind {
        NoneValueKind,
{% for item in data %}
        {{ item.name }}ValueKind,
{% endfor %}
    };

    union ValueData {
{% for item in data %}
        {{ gen_item_type_exp(item) }} m_{{ item.name }}Data;
{% endfor %}
        ValueData()
        {
        }
{% for item in data %}
        ValueData({{ gen_item_type_exp(item) }} value)
            : m_{{ item.name }}Data(value)
        {
        }
{% endfor %}
    };

    {{ name }}()
        : m_type(NoneValueKind) {
    }

{% for item in data %}
    static {{ name }} create{{ item.name }}({{ gen_item_type_exp(item) }} value)
    {
        return {{ name }}({{ item.name }}ValueKind, value);
    }

{% endfor %}
    bool isNoneValue() {
        return m_type == NoneValueKind;
    }

{% for item in data %}
    bool is{{ item.name }}Value()
    {
        return m_type == {{ item.name }}ValueKind;
    }

{% endfor %}
{% for item in data %}
    {{ gen_item_type_exp(item) }} get{{ item.name }}Value()
    {
        STARFISH_ASSERT(is{{ item.name }}Value());
        return m_data.m_{{ item.name }}Data;
    }

{% endfor %}
private:
    {{ name }}(ValueKind kind, ValueData value)
        : m_type(kind),
          m_data(value)
    {
    }

    ValueKind m_type;
    ValueData m_data;
};
}

#endif
