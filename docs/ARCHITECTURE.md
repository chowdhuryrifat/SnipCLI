# SnipCLI System Architecture & Design Blueprint

**Course:** Advanced Programming Laboratory  
**Engineers:**  
- **Lameya Tabasum** (Student ID: `250201`) - Backend & Core Architect  
- **Chowdhury Rifat Ahmed** (Student ID: `250233`) - Frontend & Routing Lead  

---

## 1. Architectural Overview: CLI MVC Pattern

In a typical GUI or web application, MVC interacts with event loops or HTTP requests. In a CLI tool like `SnipCLI`, MVC is tailored as follows:

```mermaid
flowchart TD
    User["CLI User / Terminal"] -->|argc, argv| Main["main() / CliParser"]
    Main -->|ParsedArgs| Dispatcher["CommandDispatcher (Router)"]
    
    subgraph Controller_Layer ["Controller Layer (Commands)"]
        Dispatcher -->|invokes execute()| Cmd["ICommand Implementations\n(Add, List, Get, Delete, Search, Stats)"]
    end
    
    subgraph Model_Layer ["Model Layer (Business & Data)"]
        Cmd -->|invokes| Repo["ISnippetRepository\n(JsonSnippetRepository)"]
        Cmd -->|validates| Val["SnippetValidator"]
        Cmd -->|queries| Strat["ISearchStrategy\n(TagSearch, TitleSearch)"]
        Repo -->|persists to| Storage[("snippets.json")]
    end
    
    subgraph View_Layer ["View Layer (Presentation)"]
        Cmd -->|passes DTOs to| View["ISnippetView\n(TerminalView / MarkdownView)"]
        View -->|renders text to| Stream["std::ostream\n(std::cout / std::cerr)"]
    end
```

### Strict Decoupling Rules
1. **Model has zero presentation awareness:** `Snippet`, `SnippetValidator`, and `JsonSnippetRepository` have no access to ANSI codes, format flags, or terminal widths.
2. **View has zero domain/storage awareness:** `ISnippetView` implementations receive pure DTOs/collections and format them to streams. They cannot read from disk or modify entities.
3. **Controller mediates:** `ICommand` subclasses instantiate model interactions and pass results into the injected view.

---

## 2. SOLID Principles Mapping

| Principle | Class / Interface | Application in SnipCLI |
| :--- | :--- | :--- |
| **Single Responsibility (SRP)** | `SnippetValidator`<br>`JsonSnippetRepository`<br>`TerminalView` | `SnippetValidator` only verifies business constraints.<br>`JsonSnippetRepository` only handles disk persistence.<br>`TerminalView` only converts data to ANSI strings. |
| **Open/Closed (OCP)** | `CommandDispatcher`<br>`ISearchStrategy`<br>`ViewFactory` | New commands are registered without modifying the dispatcher.<br>New search modes are plugged in via `ISearchStrategy`.<br>New view formats are added via `ViewFactory`. |
| **Liskov Substitution (LSP)** | `ISnippetRepository`<br>`ICommand` | Any repository implementation (`JsonSnippetRepository`, `InMemoryRepository` in tests) can substitute `ISnippetRepository` without breaking controller logic. |
| **Interface Segregation (ISP)** | `ISnippetReader`<br>`ISnippetWriter` | Segregated read and write interfaces prevent read-only commands (`ListCommand`, `GetCommand`) from depending on mutation methods (`save`, `remove`). |
| **Dependency Inversion (DIP)** | `AddCommand`<br>`ListCommand` | High-level commands depend on abstract `ISnippetReader` / `ISnippetWriter` and `ISnippetView`, not concrete disk files or `std::cout`. |

---

## 3. GoF Design Patterns

1. **Command Pattern (Behavioral):**
   - **Interface:** `ICommand`
   - **Concrete Commands:** `AddCommand`, `ListCommand`, `GetCommand`, `DeleteCommand`, `SearchCommand`, `StatsCommand`
   - **Invoker:** `CommandDispatcher`
   - **Benefit:** Decouples CLI parsing from business orchestration; enables adding new flags and verbs in < 5 minutes.

2. **Strategy Pattern (Behavioral):**
   - **Interface:** `ISearchStrategy`
   - **Concrete Strategies:** `TagSearchStrategy`, `TitleSearchStrategy`
   - **Context:** `SearchCommand`
   - **Benefit:** Allows dynamic runtime switching of search algorithms without touching core snippet models.

3. **Factory Pattern (Creational):**
   - **Factory:** `ViewFactory`
   - **Product Interface:** `ISnippetView`
   - **Concrete Products:** `TerminalView`, `MarkdownView`
   - **Benefit:** Isolates view creation; new presentation formats are plugged in without modifying callers.
