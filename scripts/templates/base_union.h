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
{%- macro gen_item_type_exp(item) -%}
{{ util_macro.gen_type_str(item, item.kind in non_nullable_type_kinds and item.nullable) }}
{%- endmacro -%}
#ifndef __Starfish{{ name }}__
#define __Starfish{{ name }}__

#include "binding/ScriptWrappable.h"
{% for item in include_paths %}
#include "{{item|to_h_path}}"
{% endfor %}
{% for item in used_unions %}
#include "{{item|to_union_h_path}}"
{% endfor %}

namespace Starfish {

class {{ name }} : public gc {

    // IMPORTANT NOTE !!
    //
    // This class is designed for TEMPORARY using,
    // for passing JS variables in binding code.
    // It is not recommanded to use it for other than JS binding.
    //
    // Since it does not generate a GC descriptor,
    // having a long lifetime instance can be risky.

public:
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
        ~ValueData()
        {
        }
        ValueData()
        {
#ifndef NDEBUG
            // This initialization is added for debugging purposes.(memory corruption)
            memset(this,0xFF,sizeof(ValueData));
#endif
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

    {{ name }}(const {{ name }}& src)
    {
        m_type = src.m_type;
{% for item in data %}
    {% if loop.first %}
        if(src.m_type=={{ item.name }}ValueKind){
            m_data.m_{{ item.name }}Data = src.m_data.m_{{ item.name }}Data;
        }
    {% else %}
        else if(src.m_type=={{ item.name }}ValueKind){
            m_data.m_{{ item.name }}Data = src.m_data.m_{{ item.name }}Data;
        }
    {% endif %}
{% endfor %}
        else {
            STARFISH_ASSERT(src.m_type==NoneValueKind);
        }
    }

    {{ name }}& operator=(const {{ name }}& other)
    {
        m_type = other.m_type;
{% for item in data %}
    {% if loop.first %}
        if(other.m_type=={{ item.name }}ValueKind){
            m_data.m_{{ item.name }}Data = other.m_data.m_{{ item.name }}Data;
        }
    {% else %}
        else if(other.m_type=={{ item.name }}ValueKind){
            m_data.m_{{ item.name }}Data = other.m_data.m_{{ item.name }}Data;
        }
    {% endif %}
{% endfor %}
        else {
            STARFISH_ASSERT(other.m_type==NoneValueKind);
        }
        return *this;
    }

{% for item in data %}
    static {{ name }} create{{ item.name }}({{ gen_item_type_exp(item) }} value)
    {
        return {{ name }}({{ item.name }}ValueKind, value);
    }

{% endfor %}
    bool isNoneValue() const
    {
        return m_type == NoneValueKind;
    }

{% for item in data %}
    bool is{{ item.name }}Value() const
    {
        return m_type == {{ item.name }}ValueKind;
    }

{% endfor %}
{% for item in data %}
    {{ gen_item_type_exp(item) }} get{{ item.name }}Value() const
    {
        STARFISH_ASSERT(is{{ item.name }}Value());
        return m_data.m_{{ item.name }}Data;
    }

{% endfor %}
private:
{% for item in data %}
    {% if gen_item_type_exp(item).endswith('*') %}
    {{ name }}(ValueKind kind, {{ gen_item_type_exp(item) }} value)
    {% else %}
    {{ name }}(ValueKind kind, const {{ gen_item_type_exp(item) }}& value)
    {% endif %}
        : m_type(kind),
          m_data(value)
    {
    }
{% endfor %}

    ValueKind m_type;
    ValueData m_data;
};
}

#endif
{% endcall %}

