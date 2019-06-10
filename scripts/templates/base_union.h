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

{%- macro initialize_first_member(data_list, exposed) -%}
{% for item in data_list if not item.exposed or exposed in item.exposed %}
    {% if loop.first %}
        {% if item.kind in pointer_type_kinds %}
    {{- "   : m_%sData(nullptr)"|format(item.name) -}}
        {% elif item.kind == "PrimitiveType" %}
    {{- "   : m_%sData(0)"|format(item.name) -}}
        {% elif item.kind == 'StringType' %}
    {{- "   : m_%sData(String::emptyString)"|format(item.name) -}}
        {% else %}
    {{- "   : m_%sData()"|format(item.name) -}}
        {% endif %}
    {% endif %}
{% endfor %}
{%- endmacro -%}
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
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
        {{ item.name }}ValueKind,
    {%- endcall -%}
{% endfor %}
    };

    union ValueData {
{% for item in data %}
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
        {{ gen_item_type_exp(item) }} m_{{ item.name }}Data;
    {%- endcall -%}
{% endfor %}
        ~ValueData()
        {
        }
        ValueData()
        {{ initialize_first_member(data, args.exposed) }}
        {
#ifndef NDEBUG
            // This initialization is added for debugging purposes.(memory corruption)
            memset(this,0xFF,sizeof(ValueData));
#endif
        }
{% for item in data %}
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
        ValueData({{ gen_item_type_exp(item) }} value)
            : m_{{ item.name }}Data(value)
        {
        }
    {%- endcall -%}
{% endfor %}
    };

    {{ name }}()
        : m_type(NoneValueKind) {
    }

    {{ name }}(const {{ name }}& src)
    {
        m_type = src.m_type;
        switch (m_type) {
{% for item in data %}
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
        case {{ item.name }}ValueKind:
            {% if item.name == 'Sequence' %}
            new(&m_data.m_{{ item.name }}Data) {{ gen_item_type_exp(item) }}(src.m_data.m_{{ item.name }}Data);
            {% else %}
            m_data.m_{{ item.name }}Data = src.m_data.m_{{ item.name }}Data;
            {% endif %}
            break;
    {%- endcall -%}
{% endfor %}
        default:
            STARFISH_ASSERT(src.m_type==NoneValueKind);
            break;
        }
    }

    {{ name }}& operator=(const {{ name }}& other)
    {
        m_type = other.m_type;
        switch (m_type) {
{% for item in data %}
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
        case {{ item.name }}ValueKind:
            {% if item.name == 'Sequence' %}
            new(&m_data.m_{{ item.name }}Data) {{ gen_item_type_exp(item) }}(other.m_data.m_{{ item.name }}Data);
            {% else %}
            m_data.m_{{ item.name }}Data = other.m_data.m_{{ item.name }}Data;
            {% endif %}
            break;
    {%- endcall -%}
{% endfor %}
        default:
            STARFISH_ASSERT(other.m_type==NoneValueKind);
            break;
        }
        return *this;
    }

{% for item in data %}
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
    static {{ name }} create{{ item.name }}({{ gen_item_type_exp(item) }} value)
    {
        return {{ name }}({{ item.name }}ValueKind, value);
    }

    {%- endcall -%}
{% endfor %}
    bool isNoneValue() const
    {
        return m_type == NoneValueKind;
    }

{% for item in data %}
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
    bool is{{ item.name }}Value() const
    {
        return m_type == {{ item.name }}ValueKind;
    }

    {%- endcall -%}
{% endfor %}
{% for item in data %}
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
    {{ gen_item_type_exp(item) }} get{{ item.name }}Value() const
    {
        STARFISH_ASSERT(is{{ item.name }}Value());
        return m_data.m_{{ item.name }}Data;
    }

    {%- endcall -%}
{% endfor %}
private:
{% for item in data %}
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
        {% if gen_item_type_exp(item).endswith('*') %}
    {{ name }}(ValueKind kind, {{ gen_item_type_exp(item) }} value)
        {% else %}
    {{ name }}(ValueKind kind, const {{ gen_item_type_exp(item) }}& value)
        {% endif %}
        : m_type(kind),
          m_data(value)
    {
    }
    {%- endcall -%}
{% endfor %}

    ValueKind m_type;
    ValueData m_data;
};
}

#endif
{% endcall %}

