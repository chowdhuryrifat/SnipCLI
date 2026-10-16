# SnipCLI Requirements Specification

**Course:** Advanced Programming Laboratory  
**Authors:** Lameya Tabasum (ID: `250201`) & Chowdhury Rifat Ahmed (ID: `250233`)

---

## 1. Functional Requirements (FR)

- **FR-1 (Snippet Creation):** Users must be able to create code snippets by providing title, programming language, code body (via inline flag or file input), and category tags.
- **FR-2 (Uniqueness Validation):** The system must reject duplicate snippet titles and duplicate tags within the same snippet.
- **FR-3 (Snippet Listing):** Users must be able to list all stored snippets in tabular format, with optional filtering by tag or language.
- **FR-4 (Snippet Retrieval):** Users must be able to retrieve a specific snippet by its unique numeric ID or exact title, with support for extracting raw code for terminal redirection.
- **FR-5 (Search & Filtering):** Users must be able to search snippets across titles (substring match) or category tags using dynamic algorithmic strategies.
- **FR-6 (Snippet Deletion):** Users must be able to remove snippets by ID or title.
- **FR-7 (Metric Analytics):** The system must compute and display aggregated metrics including total snippet count, total lines of code, and language distribution.
- **FR-8 (Multi-Format Rendering):** The system must support selectable presentation formats (ANSI colorized console output and GitHub-Flavored Markdown).

## 2. Non-Functional Requirements (NFR)

- **NFR-1 (Latency):** CLI command execution time must not exceed 50ms for databases containing up to 1,000 snippets.
- **NFR-2 (Memory Safety):** Zero raw owning pointers; all dynamic allocations must use `std::unique_ptr` or `std::shared_ptr`. Zero memory leaks under Valgrind/ASan.
- **NFR-3 (Portability):** Must compile without warnings under C++17 on Linux (GCC/Clang) and Windows (MSVC/MinGW).
- **NFR-4 (Extensibility - The 5-Minute Viva Constraint):** Adding a new CLI command or swapping the view output format must touch no more than 2 files and take less than 5 minutes.
