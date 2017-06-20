# How to create enum (example: SourceBuffer)

### 1. Prepare an IDL file
[SourceBuffer IDL](https://w3c.github.io/media-source/#sourcebuffer)
```c++
// SourceBuffer.idl
interface SourceBuffer : EventTarget {
  attribute AppendMode mode;
  ...
}

enum AppendMode {
    "segments",
    "sequence"
};
```

### 2. Declare enum to header file

Write enum same with the given enum in idl file.

```c++
// SourceBuffer.h
class SourceBuffer : public EventTarget {
public:
    ...
    enum AppendMode {
        Segments,
        Sequence,
    };
```

### 3. Declare getter / setter methods to header file

Declare getter / setter methods for type `String*` and type `enum` itself like below. Because even if it is required to return `enum` and get argument of type `enum`, on the script side, developer get return value of `DOMString` and provide argument value of `DOMString`. So on binding, binding codes will only use methods for type `String*`. But for internal development we also remain methods for `enum`.
(Node that, this is why we named *mode* for `String*` and *modeValue* for `enum`.)

```c++
// SourceBuffer.h
class SourceBuffer : public EventTarget {
public:
    ...
    String* mode() const
    {
        switch (m_mode) {
        case Segments:
            return String::createASCIIString("segments");
        case Sequence:
            return String::createASCIIString("sequence");
        }
        STARFISH_ASSERT_NOT_REACHED();
        return String::emptyString;
    }

    AppendMode modeValue() const
    {
        return m_mode;
    }

    void setMode(AppendMode mode);
    void setMode(String* modeStr);
};
```

### 3. Implement the rest

Implement declared methods of SourceBuffer class.