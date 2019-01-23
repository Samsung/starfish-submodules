# Acceptable extended attributes

## Interface
| Name&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; | Descripion&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; |
|----- | ----- |
| Unimplemented | Does not generate code for the unimplemented interface. |
| CustomDescriptor | Use custom descriptor.<br>All getter/setter/deleter keywords in IDL are ignored when this property exists. |
| PartialInterface | Extended interface description<br>*Ref: ChildNode.idl used by CharacterData.idl* |
| NoInterfaceObject | Interface function is not exposed to global<br>*Ref: Geolocation.idl* |
| Constructor(...) | *Ref: Event.idl* |
| NamedConstructor=X(...) | Rename construtor to X<br>*Ref: HTMLAudioElement.idl<br>`new Audio()` works while `new HTMLAudioElement()` don't* |
| CustomConstructor(...) | Does not generate contructor code. Instead, developer should write custom constructor named `XXXConstructor` to `XXXCustomBinding.cpp`<br>*Ref: Blob.idl* |
| UnimplementConstructor(...) | Does not generate constructor. |
| ConstructorCallWith=X | Native constructor will need X as a first argument.<br>X can be,<ul><li>Document (*Ref: Comment.idl*)</li><li>Window (*Ref: URL.idl*)</li></ul> |
| PrimaryGlobal | Only Window.idl need this. |
| Exposed | Exposed on a particular set of global interfaces. |

## Attribute
| Name&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; | Descripion&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; |
|----- | ----- |
| Unimplemented | Does not generate code for the unimplemented attribute. |
| Unforgeable | The attribute is owned by the object itself, not prototype.<br>*Ref: Location.idl<br>Location.prototype.hasOwnProperty("protocol") // false<br>window.location.hasOwnProperty("protocol") // true* |
| Rename=X | Corresponding native attribute will have name X<br>*Ref: Document.idl<br>charset -> characterSet* |
| RaisesException | Native code may raise exception. <ul><li>RaisesException  *(Catch exceptions for both getter, setter)*</li><li>RaisesException=Setter *(Catch exceptions for setter)*</li><li>RaisesException=Getter *(Catch exceptions for getter)*</li></ul> |
| PutForwards=X | Redirect setter to it's attribute X's setter.<br>*Ref: Document.idl<br>document.location = a ->REDIRECT-> document.location.href = a* |
| Reflect=X | Reflect the content attribute of the specified name.<br>If X is not specified, use attribute name itself.<br>*Ref: HTMLOptionElement.idl* |

## Operation
| Name&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; | Descripion&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; |
|----- | ----- |
| Unimplemented | Does not generate code for the unimplemented operation. |
| Unforgeable | The operation is owned by the object itself, not prototype.<br>*Ref: Location.idl<br>Location.prototype.hasOwnProperty("assign") // false<br>window.location.hasOwnProperty("assign") // true* |
| Rename=X | Corresponding native function will have name X |
| RaisesException | Native code may raise exception. Binding code will catch exceptions from native. |
