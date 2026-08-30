# heap

Priority queues, and a builder for them.

```cpp
ds::heap_min<std::int64_t> queue;                        // smallest on top
ds::heap_max<std::int64_t> queue;                        // largest on top
ds::heap_min<std::pair<std::int64_t, std::int64_t>> q;   // pairs order lexicographically
```

## Implementation

Aliases for `std::priority_queue` over a `std::vector`: `heap_min` supplies `std::greater`, `heap_max` takes the default. The interface is therefore `std::priority_queue`'s exactly — `push`, `pop`, `top`, `size`, `empty`, `emplace` — with no `clear()` and no `reserve()`.

| | |
| --- | --- |
| `push`, `pop` | O(log n) |
| `top`, `size`, `empty` | O(1) |
| `heapify` | O(n) |

## heapify

```cpp
auto queue = ds::heapify(values);                 // heap_min
auto queue = ds::heapify<ds::heap_max>(values);   // heap_max
auto queue = ds::heapify(std::move(values));      // takes ownership, values left empty
```

Builds the heap in O(n). Pushing the same elements one at a time costs O(n log n).

The element type is deduced from the argument; only the kind of heap is ever named.

## Traps

- `pop()` removes without returning. Read `top()` first.
- Neither alias has `clear()`. Reuse across test cases means assigning a fresh queue.
- `heap_max<T>` is exactly `std::priority_queue<T>`, so anything already written against the standard type keeps working.

## Related

- [documentation index](../README.md)
- [tests](../../tests/structures/heap.cpp)
