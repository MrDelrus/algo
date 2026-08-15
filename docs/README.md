# Documentation

One page per component, in English, under the area it belongs to.

| Area | Namespace | Alias |
| --- | --- | --- |
| [graphs/](graphs/) | `algo::graphs` | `gr` |
| [structures/](structures/) | `algo::data_structures` | `ds` |

## Page format

Every component page follows the same shape:

1. **Summary** — one sentence on what it solves.
2. **Complexity** — build / query / update, time and memory.
3. **API** — signatures, parameter semantics, index conventions.
4. **Usage** — a short, realistic snippet.
5. **Notes** — invariants, overflow and precision limits, when *not* to use it.
6. **Related** — links to sibling pages.

Conventions that hold across all pages unless a page says otherwise: vertices and array positions are 0-indexed, ranges are half-open `[left, right)`, and values are `std::int64_t`.
