# Clang Contracts Prototype

This branch contains an experimental implementation of C++ Contracts in
Clang and libc++.

The implementation targets the C++26 Contracts proposal
[P2900R14](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2900r14.pdf).
It is not yet a complete implementation of P2900R14, and it does not attempt
to implement every Contracts change in the latest C++ working draft.

Contracts are disabled by default. Enable them with `-fcontracts` and select a
translation-wide evaluation semantic with `-fcontract-mode=`:

| Option | Current behavior |
| --- | --- |
| `enforce` | Evaluate the predicate, invoke the violation handler on failure, then terminate. This is the default. |
| `observe` | Evaluate the predicate, invoke the violation handler on failure, then continue. |
| `ignore` | Do not evaluate the predicate and do not require the `std::contracts` support API. |

For example:

```sh
clang++ -std=c++2c -fcontracts -fcontract-mode=enforce example.cpp
```

The extension can also be enabled in earlier C++ language modes. When it is
enabled, Clang defines `__cpp_contracts` to `202502L`.

## Design principles and implementation choices

### P2900R14 is the implementation baseline

The language and runtime implementation are based on P2900R14, the proposal
adopted for C++26. Newer Contracts proposals are treated as follow-up work
rather than being silently folded into this implementation.

For example, the current C++29 working draft
[N5054](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5054.pdf)
contains Contracts changes that postdate P2900R14, including virtual-function
support from
[P3097R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3097r3.pdf),
which is not supported yet.

### Compatibility with the experimental GCC/libstdc++ ABI

The implementation deliberately prioritizes compatibility with the
experimental GCC/libstdc++ Contracts ABI over a paper-only compiler/library
integration model. Clang synthesizes a layout-compatible violation object and
passes it through the standard-library interface.

The current ABI uses 16-bit `assertion_kind`, `evaluation_semantic`, and
`detection_mode` enums. The private `contract_violation` representation also
contains a version, predicate text, source-location pointer, and vendor-extension
pointer.

### Translation-wide evaluation mode

The selected evaluation semantic is a language option for the entire
translation unit. Contracts-enabled state and the selected mode are part of
the compatibility signature of a PCM or module: a module cannot be imported
with a different Contracts setting or evaluation mode.

P2900R14 also defines `quick-enforce`, but Clang does not currently provide
a command-line mode that selects it.

### Ignore means no evaluation and no runtime dependency

In `ignore` mode the predicate is still parsed and semantically checked, but it
is not evaluated. Clang does not build a violation-handler call in this mode,
so the translation does not need the `std::contracts` support types to be
visible and generated code has no Contracts runtime dependency.

### Violation-handler integration

Clang first looks for an eligible, visible global
`::handle_contract_violation` with the P2900R14 signature. If none is found, it
uses a compiler builtin that lowers to the same global C++ ABI symbol.

libc++ provides a weak default `handle_contract_violation` definition. The
default handler prints the source location, assertion kind, predicate text,
and function name. A user definition can replace the weak default on supported
platforms.

### Runtime evaluation

A predicate that produces `false` is reported with
`detection_mode::predicate_false`. When C++ exceptions are enabled, an
exception escaping predicate evaluation is caught and reported with
`detection_mode::evaluation_exception`.

With `enforce`, Clang calls the handler and then terminates if the handler
returns normally. With `observe`, execution continues after a normally
returning handler. Preconditions are emitted at function entry. Postconditions
are emitted as normal cleanups after body-local cleanups and before parameter
cleanups, so they run on every normal return path.

## Known limitations

### Constant evaluation, `constexpr`, and `consteval`

Contracts are not integrated with Clang's constant evaluator yet. This has two
different observable failure modes:

- A `contract_assert` statement in a `constexpr` or `consteval` function is
  rejected as a statement that is not allowed in such a function.
- Preconditions and postconditions on a `constexpr` or `consteval` function
  are accepted, but they are not evaluated during constant evaluation. A false
  contract can therefore be silently accepted even in `enforce` mode.

### Virtual functions

Contracts on virtual functions, including overrides, are explicitly rejected.
This matches the P2900R14/C++26 implementation baseline, but it is a known gap
relative to N5054, which incorporates P3097R3 virtual-function Contracts.

### Attributes on contract assertions

The grammar permits an attribute-specifier-seq after `pre`, `post`, and
`contract_assert`, but this implementation currently requires the opening `(`
to follow immediately. Forms such as `pre [[vendor::attr]] (predicate)` are not
parsed.

### Contract equivalence across redeclarations

Currently, we'll reject:

```C++
int f() post(r: r > 0);
int f() post(r1: r1 > 0);
```

as the name are different, but we should accept. The current implementation choice
is simpler.

### `quick-enforce` cannot be selected

`evaluation_semantic::quick_enforce` is present in the library API, but
`-fcontract-mode=` accepts only `enforce`, `observe`, and `ignore`. P2900R14
allows an implementation not to expose every semantic, so this is a feature
limitation rather than necessarily a conformance defect.

### The support API must be visible in checking modes

In `observe` and `enforce` modes, the compiler currently requires the complete
`std::contracts::contract_violation`, `assertion_kind`,
`evaluation_semantic`, and `detection_mode` declarations to be visible where a
contract assertion is processed. In practice, source using checked Contracts
should include `<contracts>`. This requirement does not apply to `ignore` mode.

### Windows support

We didn't design for windows nor tested.

### Tooling coverage

Some tooling paths do not yet treat function preconditions and postconditions
as ordinary traversable AST children. AST visitors, source rewriting, and AST
printing may therefore need Contracts-specific handling. In particular,
printing inherited contracts from a redeclaration whose parameter names differ
can produce source that is not valid for that redeclaration.
