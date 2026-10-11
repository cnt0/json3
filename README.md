# JSON3

**WIP**

Streaming malloc-free JSON parser in C.

Uses the original state machine [written](https://github.com/douglascrockford/JSON-c) by JSON creator (it was right there all this time).

Implements approach similar to [JSMN](https://github.com/zserge/jsmn), i.e. the parsed state is an array of simple structures which can be (de)serialized just like that, using `mmap` and stuff. jsmn seems to be abandoned though.

Features I'd like to implement:
- header-only
- license
- proper test suite
- examples
- compressed and deduplicated representation of token array
- maybe functions for mutation (they'll work in O(n))
- maybe some sane subset of [JSON Schema](https://json-schema.org/specification) or some better spec which will allow to implement the `json-scanner`, [JSON Pointer](https://www.rfc-editor.org/info/rfc6901), etc.

"3" means 3rd version of this project (I wasn't happy with the previous 2, so decided not to publish and just nuked them).

