# How to create interface

### Procedures
* Find idl on the web, if there're duplicated spec, then put much priority `whatwg` to `w3c`.
* Locate idl where you want to implement `interface`, and modify idl conforming to our style, and mark attributes if needs.
* Add module in macro `STARFISH_ENUM_LAZY_BINDING_NAMES`. Currently you can find this `ScriptBindingInstance.h`, but it is now under development, so maybe it can be located in different file.
* Generate class according to idl. please refer to [Type Conversion](#type-conversion), [Header Hierarchy](#header-hierarchy) and [Document and StarFish Hodable](#document-and-starfish-holdable)
* Declare `virtual void init(ScriptBindingInstance* instance) override` and `virtual bool is###() const override` functions.

### Type Conversion
| Type from idl | Type from native |
| ----- | ----- |
| boolean | bool |
| byte | int8_t |
| octet | uint8_t |
| short | int16_t |
| unsigned short | uint16_t |
| long | int32_t |
| unsigned long | uint32_t |
| long long | int64_t |
| unsigned long long | uint64_t |
| float, unrestricted_float, double, unrestricted_double | double |
| DOMString | String*(as arguments) |
| object | ScriptObject |
| any | ScriptValue |
| Sequence<T> | GCVector<T> |
| dicrionary | [How to create dictionary](/doc/HowToCreateDictionary.md) |
| enum | [How to create enum](/doc/HowToCreateEnum.md) |
| XXX | XXX* (e.g. EventTarget -> EventTarget*, Node -> Node*) |
| *? | Wrap Nullable<T> if T is primitive type, DOMString or dictionary, otherwise use pointer as raw (e.g, DOMString should be converted to `Nullable<String*>`) |

### Header Hierarchy
Basically every interface inherits `ScriptWrappable`. Starting from `ScriptWrappable`, the inheritance flows down to the specific interface. if the `interface` you want to implement has a `parent interface`, then it is enough to include parent, nothing more needed. Otherwise just include `ScriptWrappable`. What if there are some `interfaces` given as `TypeRef(such as pointer referencing to interface)` or `Dictionary`, then please use `Forward Declaration` as possible.(In some cases, we can't use `Forward Declaration`, if we use some type as `value type(Type a;)` or `reference type(Type& a;)` from native side) For example, if you have a idl like below.
```
[Exposed=Window]
interface HTMLCollection {
  readonly attribute unsigned long length;
  getter Element? item(unsigned long index);
  [NotEnumerable] getter Element? namedItem(DOMString name);
};
```
Then from native side, the interface should be like below.
```
#ifndef __StarFishHTMLCollection__
#define __StarFishHTMLCollection__

#include "binding/ScriptWrappable.h" // (1) This is the `interface` without `parent`, so it has to inheirt `ScriptWrappable`.
#include "dom/NodeList.h" // (2) Due to internal implmentation
#include "dom/NodeListImpl.h" // same with (2)

namespace StarFish {

class Node; // (4) same with (2)
class Element; // (5) We forward delcared `TypeRef` type `Element`.

class HTMLCollection : public ScriptWrappable { // (6) same with (1)
  size_t length() const;
  Element* item(unsigned long index);
  Element* namedItem(String* name);
}
```
### Document and StarFish Holdable
With 2 reasons, 1. to remove duplicative long expression to call `window` or `starfish` through `document` such as `document()->window()->starFish()`, 2. to remove abuse of holding member `document` or `starFish`, we implemented simple common pattern to hold `Document` or `StarFish`. So if any class uses either of ones, please inherit this holdable class. For example, `EventTarget` needs to hold `document` inside so instead we implement getter/setter of `document`, we just inherit `DocumentHoldable`, then it's done!

```c++
class EventTarget : public ScriptWrappable, public DocumentHoldable {
protected:
    EventTarget(Document* document)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
    {
    }

```