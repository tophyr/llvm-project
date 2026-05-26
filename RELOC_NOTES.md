# Relocation Proposal Notes

This tree implements most of the relocation proposal directly, but a few parts
of the paper either required implementation-specific choices or exposed design
weaknesses.

## `std::construct_at(T*, T)` needs compiler magic

The paper's C++26 relocation overload for `std::construct_at` is:

```cpp
template<class T>
constexpr T* construct_at(T* p, T src);
```

with calls such as:

```cpp
std::construct_at(p, reloc src);
```

As ordinary library code, that signature is not enough to express the intended
operation. A normal function body cannot spell "invoke `T`'s relocation
constructor with `p` as the destination `this` object directly from the
caller's relocation source". If implemented naively, the by-value parameter
creates an extra intermediate object and therefore an extra relocation step.

To preserve the exact paper surface anyway, this tree uses brittle compiler
magic:

- Clang recognizes the exact `std::construct_at(T*, T)` overload in namespace
  `std`.
- Direct calls of the form `std::construct_at(p, reloc x)` are lowered
  specially so the destination object at `p` is initialized directly from the
  original relocation source.
- libc++ uses the internal helper builtin `__builtin_construct_at_reloc(...)`
  in the function body so the exact signature remains valid source code.

This keeps the paper's API spelling, but it is not a normal library-only
feature.

## Structured decomposition needed an explicit `reloc` marker

The paper's structured decomposition wording reuses ordinary structured binding
syntax:

```cpp
auto [x, y] = foo();
```

and gives it decomposition semantics when the decomposition protocols match.
That would silently change the meaning of existing valid C++.

This implementation does **not** do that. It requires an explicit marker:

```cpp
auto [x, y] reloc = foo();
```

This was a deliberate compatibility deviation. Without a marker, the proposal
would reinterpret existing code into a different feature.

## Structured decomposition names are object-like, not ordinary bindings

The paper says the declared names of a structured decomposition are complete
subobjects. That does not fit Clang's ordinary `BindingDecl` model, where a
structured binding name is more like a projection or alias into another object.

This implementation treats explicit reloc decomposition names as object-like
entities with dedicated relocation semantics, rather than trying to model them
as ordinary structured bindings with a few extra flags.

## `reloc` does not come with a full forwarding model

The paper gives `reloc` useful by-value transport rules, but it does not define
an equivalent of `T&&` plus `std::forward`. In practice, `T reloc` parameters
and `reloc expr` arguments do not compose with generic forwarding in the same
way forwarding references do.

That weakness is mostly paper design rather than implementation, but it showed
up concretely while working through the `construct_at` wording above.

## Virtual slicing is ABI-affecting

The paper leaves the virtual slicing function's parameters implementation
defined. Clang therefore had to choose a concrete ABI surface.

Current status in this tree:

- a concrete hidden virtual slicing function was implemented for both the
  Itanium and MS C++ ABIs
- the exact ABI shape remains an implementation choice rather than something
  fully specified by the paper

That is an implementation choice rather than a paper deviation, but it is still
important because it affects object layout and virtual dispatch ABI.

### Current incompatibility

The current Itanium implementation adds a new hidden virtual member to eligible
classes. That is **not** generally ABI-compatible with older Itanium-built
code.

In particular:

- vtable layout changes for affected polymorphic classes
- existing virtual slot numbering can change
- old code that never explicitly uses relocation can still become incompatible
  if it interacts with those classes across an ABI boundary
- linking old clients against a newly-built library is therefore not safe in
  the general case for affected classes

So the current implementation should be viewed as an ABI break for eligible
polymorphic classes, not as a purely additive extension.

### Compatibility-preserving directions

If backward compatibility is a hard requirement, the virtual slicing design
should avoid changing existing class layout or primary vtable layout. The main
options are:

- no new virtual slot at all: perform virtual slicing through external runtime
  logic keyed off RTTI or other type metadata
- side metadata instead of the primary vtable: emit an auxiliary relocation
  descriptor associated with the type and let new code probe for it
- explicit opt-in ABI: only classes compiled with a dedicated attribute or flag
  participate in the new slicing ABI
- hidden per-type helper symbols: emit out-of-line slicing helpers that new
  code can discover and call without modifying existing vtable layout

The common rule is that additive compatibility requires keeping existing object
layout and existing virtual slot numbering unchanged. The current hidden-virtual
design does not satisfy that rule.
