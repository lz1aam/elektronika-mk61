# Contributing

Bug reports, verified calculator behavior, reference photographs, and focused
code changes are welcome. Please keep the interface faithful to the physical
Elektronika MK-61 and preserve the microcode-based execution model.

Before submitting a change:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Document the source of expected calculator behavior or visual measurements.
Do not add ROMs, fonts, photographs, or other third-party material without a
clear redistribution basis. By contributing, you agree that your contribution
may be distributed under GPL-3.0.
