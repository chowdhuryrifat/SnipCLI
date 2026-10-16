### Summary of Changes
- **Category:** [ ] Feature [ ] Bug Fix [ ] Refactoring [ ] Documentation [ ] Testing [ ] CI
- **Description:** Provide a concise summary of changes and rationale.
- **Related Issue:** Closes #

### Architectural & Layer Compliance
- [ ] **Model Layer:** Free of presentation logic, ANSI escape codes, or CLI argument handling.
- [ ] **View Layer:** Free of business logic, database queries, and direct model mutations.
- [ ] **Controller Layer:** Mediates between Model and View via Command Pattern.
- [ ] **SOLID Principles:** Verified compliance with SRP, OCP, LSP, ISP, and DIP.
- [ ] **Design Patterns:** Proper usage of Command / Strategy / Factory patterns.

### Code Quality & Standards (`RULES.md`)
- [ ] Code adheres strictly to `RULES.md` and passes `.clang-format`.
- [ ] Doxygen documentation provided for all new classes, interfaces, and methods.
- [ ] `const` correctness verified across all parameters and member functions.
- [ ] Zero raw owning pointers (`std::unique_ptr` / `std::shared_ptr` used).
- [ ] RAII applied for all system resources.

### Local Verification
- [ ] Clean build with zero warnings: `cmake --build build --config Release`
- [ ] Unit tests passing: `ctest --test-dir build --output-on-failure`
- [ ] Smoke tests passing: `./tests/smoke/smoke_test.sh ./build/snip`
- [ ] Reviewed and signed off by peer developer.
