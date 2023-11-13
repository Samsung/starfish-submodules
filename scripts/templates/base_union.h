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
{%- macro gen_item_type_exp(item) -%}
{{ util_macro.gen_type_str(item, util_macro.is_non_nullable_type(item.kind) and item.nullable) }}
{%- endmacro -%}
#ifndef __Starfish{{ name }}__
#define __Starfish{{ name }}__

#include "binding/ScriptWrappable.h"
{% for item in used_unions|sort %}
#include "{{item|to_union_h_path}}"
{% endfor %}

{%- macro initialize_first_member(data_list, exposed) -%}
{% for item in data_list if not item.exposed or exposed in item.exposed %}
    {% if loop.first %}
        {% if item.kind in pointer_type_kinds %}
    {{- "   : m_%sData(nullptr)"|format(util_macro.getTrimmedName(item)) -}}
        {% elif item.kind == "PrimitiveType" %}
    {{- "   : m_%sData(0)"|format(util_macro.getTrimmedName(item)) -}}
        {% elif item.kind == 'StringType' %}
    {{- "   : m_%sData(String::emptyString)"|format(util_macro.getTrimmedName(item)) -}}
        {% else %}
    {{- "   : m_%sData()"|format(util_macro.getTrimmedName(item)) -}}
        {% endif %}
    {% endif %}
{% endfor %}
{%- endmacro -%}

{% macro has_sequence_type(list) %}
    {% for x in list %}
        {% if x.kind.startswith('SequenceOf') %}
            {{ True }}
        {% endif %}
        {% if x.data %}
            {% if x.data.name in has_sequence_type_kinds %}
                {{ True }}
            {% endif %}
        {% endif %}
    {% endfor %}
{% endmacro %}

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
        {{ util_macro.getTrimmedName(item) -}}ValueKind,
    {%- endcall -%}
{% endfor %}
    };

    union ValueData {
{% for item in data %}
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
        {{ gen_item_type_exp(item) }} m_{{- util_macro.getTrimmedName(item) -}}Data;
    {%- endcall -%}
{% endfor %}
        ~ValueData()
        {
        }
        ValueData()
        {{ initialize_first_member(data, args.exposed) }}
        {
        {% if has_sequence_type(data) %}
            // memory blocks need to be initialized before making an empty vector
            memset(this,0x00,sizeof(ValueData));
        {% else %}
#ifndef NDEBUG
            // This initialization is added for debugging purposes.(memory corruption)
            memset(this,0xFF,sizeof(ValueData));
#endif
        {% endif %}
        }
{% for item in data %}
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
        ValueData({{ gen_item_type_exp(item) }} value)
            : m_{{- util_macro.getTrimmedName(item) -}}Data(value)
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
        case {{ util_macro.getTrimmedName(item) -}}ValueKind:
            {% if item.name == 'Sequence' %}
            new(&m_data.m_{{- util_macro.getTrimmedName(item) -}}Data) {{ gen_item_type_exp(item) }}(src.m_data.m_{{- util_macro.getTrimmedName(item) -}}Data);
            {% else %}
            m_data.m_{{- util_macro.getTrimmedName(item) -}}Data = src.m_data.m_{{- util_macro.getTrimmedName(item) -}}Data;
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
        case {{ util_macro.getTrimmedName(item) -}}ValueKind:
            {% if item.name == 'Sequence' %}
            new(&m_data.m_{{- util_macro.getTrimmedName(item) -}}Data) {{ gen_item_type_exp(item) }}(other.m_data.m_{{- util_macro.getTrimmedName(item) -}}Data);
            {% else %}
            m_data.m_{{- util_macro.getTrimmedName(item) -}}Data = other.m_data.m_{{- util_macro.getTrimmedName(item) -}}Data;
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
    static {{ name }} create{{- util_macro.getTrimmedName(item) -}}({{ gen_item_type_exp(item) }} value)
    {
        return {{ name }}({{- util_macro.getTrimmedName(item) -}}ValueKind, value);
    }

    {%- endcall -%}
{% endfor %}
    bool isNoneValue() const
    {
        return m_type == NoneValueKind;
    }

{% for item in data %}
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
    bool is{{- util_macro.getTrimmedName(item) -}}Value() const
    {
        return m_type == {{- util_macro.getTrimmedName(item) -}}ValueKind;
    }

    {%- endcall -%}
{% endfor %}
{% for item in data %}
    {%- call util_macro.ifdef_and_exposed(item.flags, item.exposed, args.exposed) %}
    {{ gen_item_type_exp(item) }} get{{- util_macro.getTrimmedName(item) -}}Value() const
    {
        STARFISH_ASSERT(is{{- util_macro.getTrimmedName(item) -}}Value());
        return m_data.m_{{- util_macro.getTrimmedName(item) -}}Data;
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

