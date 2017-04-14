{% for form in function.operations|default([]) %}
static ESValue {{ function.name }}Form{{loop.index}}(ESVMInstance* instance)
{
    return ESValue(ESValue::ESNull);
}

{% endfor %}
static ESValue {{ function.name }}Function(ESVMInstance* instance)
{
    return ESValue(ESValue::ESNull);
}

