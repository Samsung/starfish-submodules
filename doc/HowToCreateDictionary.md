# How to create dictionary (example: EventInit)

### 1. Prepare an IDL file
[EventInit IDL](https://dom.spec.whatwg.org/#dictdef-eventinit)
```c++
// EventInit.idl
dictionary EventInit {
  boolean bubbles = false;
  boolean cancelable = false;
  boolean composed = false;
};
```
Put IDL contents into existing file such as `Event.idl`,  
or simply create a new file `EventInit.idl` to put in.  

### 2. Declare struct to header file  
A `dictionary` represents `struct` in native code.  
Write corresponding struct to the header file.  
(Note that, header file must be named as same as your IDL file name)  

```c++
// EventInit.h
struct EventInit {
public:
  // Single Constructor without parameter
  EventInit();

  // Member getters
  bool bubbles() const;
  bool cancelable() const;
  bool composed() const;

  // Member setters
  void setBubbles(bool bubbles);
  void setCancelable(bool cancelable);
  void setComposed(bool composed);

    ...
};
```

### 3. Implement the rest  
Implement declared methods of EventInit struct.  