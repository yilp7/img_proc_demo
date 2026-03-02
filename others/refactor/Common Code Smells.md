# Common Code Smells

In software engineering, certain recurring bad practices are known as "anti-patterns" or "code smells." These indicate deeper architectural problems in a codebase. Here is a breakdown of the most common ones.

## 1. The God Class (or God Module)

A **God Class** refers to a class or module that has grown so large and complex that it tries to "know everything" and "do everything" in the application. Instead of delegating tasks to other parts of the system, it hoards data and centralizes control.

### Common Characteristics

You can usually spot a God Class by looking for a few telltale signs:

- **Massive Size:** It often contains thousands of lines of code and dozens (or hundreds) of methods and variables.
- **Too Many Responsibilities:** It handles user authentication, database connections, data formatting, and business logic all in one place.
- **High Coupling:** It is deeply entangled with almost every other part of the system.
- **Vague Names:** They are frequently given all-encompassing names like `SystemManager`, `ApplicationCore`, `GlobalController`, or `MainHelper`.

### Why It's a Nightmare

- **Hard to Maintain:** Changing one small feature can accidentally break three unrelated features because everything is intertwined.
- **Testing is Nearly Impossible:** Writing automated tests requires a massive amount of setup and mocking.
- **Team Bottlenecks:** Multiple developers will often need to edit it at the same time, leading to constant Git merge conflicts.

> **The Core Violation:** A God Class is the ultimate violation of the **Single Responsibility Principle (SRP)**. A class should have one, and only one, specific job and reason to change.

### How to Fix It

Curing a God Class requires **refactoring**—dismantling it piece by piece:

1. **Identify Responsibilities:** Group the methods by what they actually do.
2. **Extract Classes:** Move those grouped methods and their associated data into new, smaller, focused classes (e.g., `UserRepository`, `EmailService`).
3. **Delegate:** Update the original God Class to delegate the work to these new, specialized classes instead of doing it itself.

## 2. Shotgun Surgery

**Shotgun Surgery** is essentially the exact opposite of a God Class. While a God Class crams too many responsibilities into a *single* file, **Shotgun Surgery happens when a single responsibility or concept is spread out across** ***many*** **different files or classes.**

### Why It's a Problem

It gets its name because making one simple logical change requires you to hunt down and make a bunch of tiny edits scattered all over your codebase—like the wide spread of pellets from a shotgun blast.

- **Highly Brittle Code:** It is incredibly easy to miss one of the required changes in a distant file, leading to bugs.
- **Cognitive Load:** Developers have to keep a mental map of everywhere a concept is touched across the entire system.

### How to Fix It

Instead of splitting things apart (as you do with a God Class), you bring them together using a technique called **Move Method** or **Move Field**. You identify the scattered behavior that changes together and consolidate it into a single, cohesive class or module.

## 3. Feature Envy

**Feature Envy** occurs when a method in one class is "envious" of the data or methods in another class.

You can usually spot this when a method spends more time interacting with another object than it does with its own data. For example, if `Class A` calls five different "getter" methods on `Class B` just to calculate a total or format a string, `Class A` is exhibiting Feature Envy.

### Why It's a Problem

It violates the core object-oriented principle of **encapsulation**, which states that data and the behavior that operates on that data should be kept together. When logic is separated from its data, the code becomes harder to read, and the classes become tightly coupled (meaning a change to `Class B`'s data structure will easily break `Class A`).

### How to Fix It

The standard solution is to move the "envious" method (or the specific chunk of logic doing the calculation) over to the class that actually owns the data. If `Class A` is constantly asking `Class B` for data to do math, you should just have `Class B` do the math and return the final result.

## The Balancing Act: Feature Envy vs. God Class

If you fix Feature Envy by moving every envious method into the class that holds the data, **won't that data class easily become a God Class?**

Yes! This is one of the biggest architectural tightropes in software engineering. If `Class B` (a User class, for example) holds a lot of data, and you move the logic for database saving, JSON serialization, and UI formatting into the User class just because they use User data, your User class will quickly turn into a God Class.

Here is how you resolve this tension:

1. **Check the Responsibility:** Before moving an envious method, ask: *Does this behavior belong to the core responsibility of the target class?*
    - **Yes:** If it's a method calculating a User's full name from their first and last name, move it to the `User` class.
    - **No:** If it's a method saving the User to a database, that is a *different responsibility*. Do not put it in the `User` class.
2. **Extract a Third Class:** If a method belongs to a different responsibility, but it still heavily envies data from another class, the solution is usually to create a **brand new class** that sits between them.
    - For example, instead of putting database logic in the `User` class (creating a God Class) or putting it in a generic `DatabaseManager` (creating Feature Envy), you create a specific `UserRepository` class. This new class has the single responsibility of handling User database interactions, beautifully balancing both concerns!

## 4. Spaghetti Code

**Spaghetti Code** is perhaps the most famous of all software anti-patterns. It refers to source code that has a complex, tangled, and unpredictable control flow, making it look and behave like a bowl of spaghetti. If you pull on one noodle (change one piece of code), it's impossible to predict which other noodles will move.

### Common Characteristics

- **Unpredictable Control Flow:** Historically, this was caused by the overuse of `GOTO` statements. Today, it usually looks like deeply nested `if/else` conditions, massive `switch` cases, and sprawling loops that jump erratically.
- **Lack of Structure:** There is no clear architecture, separation of concerns, or modularity. Functions, variables, and classes bleed into one another without boundaries.
- **Global Variables:** Heavy reliance on global state that can be modified by any part of the program at any time, making it impossible to track down exactly where a value was changed.

### Why It's a Problem

- **Impossible to Trace:** Debugging is a nightmare because you cannot easily follow the execution path from start to finish.
- **Fear of Changing Code:** Developers become terrified to touch the codebase because making a seemingly simple fix almost always introduces unexpected bugs somewhere else.
- **Onboarding Nightmare:** New developers take weeks or months to understand how the system works because there is no logical flow or structure to follow.

### How to Fix It

Untangling spaghetti code is a tedious but absolutely necessary process.

- **Untangle the Logic:** Flatten deeply nested `if/else` conditions by using techniques like "Early Returns" (Guard Clauses).
- **Modularize:** Extract chunks of tangled logic into small, well-named functions with clear inputs and outputs.
- **Remove Global State:** Pass variables explicitly as arguments to functions instead of relying on global variables.
- **Write Tests First:** Before making any changes, try to write unit tests for the existing behavior. This provides a safety net so you know you aren't breaking existing functionality while untangling the code.

## 5. Copy-Paste Programming

**Copy-Paste Programming** happens when a developer duplicates existing code blocks to implement a new, similar feature rather than creating a reusable function or component.

### Common Characteristics

- **Heavy Code Duplication:** You see the exact same 20 lines of code in five different files, perhaps with only one or two variables changed.
- **"Cargo Cult" Programming:** Developers paste code they don't fully understand just because "it worked over there," sometimes carrying over completely irrelevant logic.
- **Ghost Bugs:** You fix a bug in one place, but users keep reporting it because the buggy code was pasted into three other files that you forgot to update.

### Why It's a Problem

> **The Core Violation:** This is a direct violation of the **DRY Principle** (Don't Repeat Yourself).

- **Maintenance Multiplier:** If you need to change a core rule (like how a tax calculation works) and that logic has been copy-pasted in 10 places, you now have 10 places to find, update, and test. If you miss even one, your application is now inconsistent and broken.
- **Code Bloat:** It rapidly inflates the size of the codebase, making it harder to navigate and slower to compile or download.
- **False Context:** Pasted code often contains comments or variable names from its original context that make absolutely no sense in its new home, causing confusion for future developers.

### How to Fix It

The solution requires taking a step back to find the common patterns before writing the new feature:

- **Extract Method/Function:** If you are about to copy and paste a block of logic, stop. Instead, pull that logic out into its own separate, reusable function. Give it a clear name and define the parameters it needs to handle the slight variations. Then, simply *call* that function from both the old location and your new location.
- **Use Design Patterns:** For object-oriented programming, leverage tools like Inheritance, Interfaces, or Composition to share behavior across different classes instead of copying methods.

## 6. Magic Numbers and Magic Strings

**Magic Numbers** (or Magic Strings) are hard-coded values used directly in the source code without any explanation of what they mean or why they are there. They appear as if by "magic," leaving future developers to guess their purpose.

### Common Characteristics

- **Unexplained Values:** You see logic like `if (status == 2)` or `salary * 0.0825`. What is `2`? What is `0.0825`? The code doesn't say.
- **Hard-Coded Identifiers:** Using raw strings for roles or statuses, like `if (role == "super_admin")` scattered across dozens of files.
- **Hidden Math:** Seeing values like `86400` instead of calculating it from logical components (e.g., seconds in a day).

### Why It's a Problem

- **Zero Context:** A number by itself carries no meaning. A developer reading `if (password.length > 8)` might guess it's a security rule, but `if (user.type == 4)` is completely impossible to decipher without digging through database documentation.
- **The "Shotgun Surgery" Trigger:** If the sales tax changes from `0.0825` to `0.085`, or if the `"super_admin"` role is renamed to `"owner"`, you have to do a global find-and-replace. This is incredibly dangerous, as you might accidentally replace a `0.0825` that meant something entirely different.
- **Typo Susceptibility:** If you misspell `"super_admin"` as `"supper_admin"` in one random file, the compiler won't catch it, but that user will mysteriously lose access.

### How to Fix It

The fix is one of the easiest and most satisfying refactoring tasks: **Replace Magic Values with Named Constants or Enums.**

- **Extract Constants:** Take `86400` and create a clearly named constant at the top of your file (or in a shared constants file): `const SECONDS_IN_A_DAY = 86400;`. Then use `SECONDS_IN_A_DAY` in your logic.
- **Use Enums:** For states or types (like `2` or `"super_admin"`), use Enumerations: `if (status == Status.DELIVERED)` or `if (role == Role.SUPER_ADMIN)`.
- **Show the Math:** Instead of `86400`, write `60 * 60 * 24`. The compiler will optimize it into a single number anyway, but human readers will instantly understand you mean "60 seconds * 60 minutes * 24 hours".

## 7. Missing Abstraction Layer

**Missing Abstraction Layer** occurs when high-level business logic is directly intertwined with low-level implementation details, such as raw API calls, explicit database queries, or third-party library specifics, rather than communicating through a clean, generic interface.

### Common Characteristics

- **Raw Library Calls Everywhere:** Seeing `axios.post('https://api.example.com/users')` directly inside UI components across 30 different files.
- **Leaking Implementation Details:** Controllers or business services manually constructing SQL queries or managing TCP sockets instead of asking a data access layer to do it.
- **No Centralized Configuration:** Having to update URLs, API keys, or database schemas in a dozen places when a minor structural change happens.

### Why It's a Problem

- **Vendor Lock-In:** If you decide to switch from `axios` to `fetch`, or from PostgreSQL to MongoDB, you will have to rewrite huge portions of your application because the specific library is hard-coded everywhere.
- **Testing Complexity:** It is difficult to unit test business logic when it makes hard-coded external calls. You are forced to mock the exact HTTP library or database driver instead of just mocking a simple interface.
- **Violation of Separation of Concerns:** High-level code (like "create a user profile") shouldn't have to care about HTTP headers, JSON parsing, or database connection strings.

### How to Fix It

Introduce an abstraction layer (often using the **Facade**, **Adapter**, or **Repository** patterns) to separate the "what" from the "how."

- **Create Wrappers:** Instead of calling the HTTP library directly in components, create an `ApiClient` class that handles the low-level `fetch` or `axios` work. Have all components call `ApiClient.get('/users')`.
- **Use Repositories:** Move all raw SQL or ORM queries into dedicated Repository classes (e.g., `UserRepository.save(user)`). Your business logic only talks to the Repository.
- **Depend on Abstractions:** Define interfaces (like `IEmailService`) and program against them. Inject the concrete implementation (like `SendGridEmailService`) at runtime, making it trivial to swap providers later.

## 8. Circular Dependencies

A **Circular Dependency** occurs when two or more modules, classes, or packages depend on each other directly or indirectly. For example, `Module A` requires `Module B` to function, but `Module B` also requires `Module A`.

### Common Characteristics

- **The Import Loop:** In languages like JavaScript/TypeScript or Python, you see `import { B } from './B'` inside file A, and `import { A } from './A'` inside file B.
- **The Infinite Constructor:** `Class A` instantiates `Class B` inside its constructor, and `Class B` instantiates `Class A` inside *its* constructor.
- **A -> B -> C -> A:** The dependency doesn't have to be direct. Sometimes A depends on B, B depends on C, and C loops all the way back to depend on A.

### Why It's a Problem

- **The Chicken-and-Egg Initialization Problem:** The compiler or runtime environment doesn't know which file to load first. If it tries to load A, it has to load B. But to load B, it has to load A. This often results in mysterious application crashes, `StackOverflow` errors, or variables silently evaluating to `undefined` during startup.
- **Impossible to Isolate:** You can never reuse `Module A` in another project without also dragging along `Module B` (and vice versa). They are effectively fused together into one giant, two-headed module.
- **Ripple Effects:** Changes in one module are highly likely to break the other, creating a fragile architecture.

### How to Fix It

Fixing a circular dependency involves breaking the loop by rearranging how the modules talk to each other.

- **Extract a Third Module:** If A and B depend on each other because they both need access to a shared piece of logic or data, extract that shared piece into a brand new `Module C`. Then, have both A and B depend on C. The circle is broken!
- **Use Dependency Inversion (Interfaces):** Instead of `Class A` depending on the concrete `Class B`, have `Class A` depend on an *Interface*. `Class B` can then implement that interface. This flips the direction of the dependency.
- **Merge Them:** Ask yourself: *Do these two things actually need to be separate?* If A and B are so heavily intertwined that they cannot exist without one another, you might have unnecessarily split a single concept in half. The easiest fix might be to merge them into a single, cohesive class or file.

## 9. Excessive Coupling (or Tight Coupling)

**Excessive Coupling** happens when two or more classes, modules, or services are overly dependent on the internal workings of one another. In a tightly coupled system, the components are glued together so strictly that they lose their independence.

### Common Characteristics

- **Direct Instantiation:** `Class A` creates `Class B` directly inside itself using the `new` keyword (e.g., `const db = new DatabaseConnection()`) instead of having it passed in from the outside.
- **Knowing Too Much:** Violating the **Law of Demeter** ("Don't talk to strangers") by chaining methods deeply. For example: `customer.getAccount().getHistory().getLastTransaction()`. Here, the code calling this knows intimately about the internal structure of the `Customer`, `Account`, and `History` classes.
- **Depending on Concrete Implementations:** Relying on a specific class like `MySQLDatabase` rather than a general interface like `IDatabase`.

### Why It's a Problem

- **Zero Reusability:** If `OrderService` is tightly coupled to `EmailNotificationService`, you can never reuse the `OrderService` in an environment where you want to send SMS notifications instead. You always have to drag the email logic with you.
- **Testing Nightmares:** It is incredibly difficult to unit test tightly coupled code. If you try to test `Class A`, you are forced to spin up the entire setup for `Class B` (which might hit a real database or real API) because you cannot easily replace `Class B` with a mock or fake version.
- **Cascading Failures:** A change to the internal workings of one class necessitates changes across many other classes, directly leading to Shotgun Surgery.

### How to Fix It

Decoupling is one of the core goals of good software architecture.

- **Dependency Injection (DI):** Instead of `Class A` creating `Class B` internally, pass `Class B` into `Class A` via its constructor or a method parameter. This allows you to swap out dependencies effortlessly (especially useful for injecting mock objects during testing).
- **Program to Interfaces, Not Implementations:** Depend on abstractions. If your class requires a database connection, depend on an `IDatabase` interface, not a specific `PostgresDatabase` class.
- **Use Event-Driven Architecture:** If components just need to know when something happens, use a Publish/Subscribe (Pub/Sub) model or an Observer pattern. `Class A` simply shouts "An order was placed!" and doesn't care who is listening. `Class B` listens for that event and reacts independently.

### 💡 Note: Feature Envy vs. Excessive Coupling

Because they sound similar, these two concepts are often confused. Here is the easiest way to remember the difference:

- **Feature Envy** is about *misplaced logic*. It happens when a method in Class A acts like a micromanager, constantly pulling raw data out of Class B to do work that Class B should have done itself.
- **Excessive Coupling** is a broader architectural term about *rigid connections*. It means two classes are glued together so tightly that they cannot be separated, reused, or tested independently.

**The Relationship:** Feature Envy automatically creates Excessive Coupling (because Class A is forced to know Class B's internal data structure). However, *not all Excessive Coupling is Feature Envy* (e.g., hard-coding a specific database connection is Excessive Coupling, but it isn't Feature Envy).

## 10. Patch Artifacts

**Patch Artifacts** refer to the leftover remnants of temporary fixes, debugging sessions, or "band-aid" workarounds that get left behind in the codebase. What was intended to be a quick, temporary patch ends up becoming a permanent, confusing fixture.

### Common Characteristics

- **Zombie Code (Commented-Out Code):** Large blocks of code that have been commented out with no explanation as to why they are commented or when they should be removed.
- **Forgotten Debug Statements:** `console.log`, `print()`, or debug alerts that accidentally get pushed to production.
- **Ancient `TODO`s:** Comments like `// TODO: fix this later, hack for now` that have sat untouched in the code for years.
- **Temporary Boolean Bypasses:** Logic like `if (userId == 1234) { return true; }` that was added to quickly fix a bug for a specific customer or during a demo, but never generalized or removed.

### Why It's a Problem

- **Clutter and Confusion:** Future developers are terrified to delete zombie code because they don't know if it's broken, deprecated, or if someone was planning to turn it back on.
- **The "Broken Window" Theory:** If developers see a codebase littered with leftover debug statements and ancient `TODO`s, they subconsciously assume the code quality standards are low, making them more likely to write sloppy code themselves.
- **Hidden Bugs:** A temporary patch almost never addresses the root architectural issue. Relying on these band-aids means the underlying bug is still waiting to explode under stress.

### How to Fix It

- **Trust Version Control:** Delete commented-out code immediately! Git remembers everything. If you ever need that logic back, you can look through your commit history. You don't need to keep it in the live file.
- **Use Automated Linters:** Set up pre-commit hooks or CI/CD pipelines that automatically reject code containing `console.log` or forgotten debug statements.
- **Fix the Root Cause:** Whenever you find a temporary bypass (`if (isHack) { ... }`), take the time to map out the *actual* architectural fix needed, create a proper ticket for it, and resolve the underlying issue.

## 11. Dead Code

**Dead Code** refers to functions, variables, parameters, or entire blocks of logic that are present in the codebase but are either never executed or their execution has no effect on the application.

### Common Characteristics

- **Unreachable Code:** Logic placed after a `return`, `throw`, or `break` statement, or placed inside an `if (false)` block, making it physically impossible for the compiler to ever reach it.
- **Unused Variables and Functions:** Variables that are declared but never read, or helper functions that were written for a feature that has since been deleted.
- **Redundant Computations:** Code that successfully calculates a value or performs an operation, but that final result is never actually passed anywhere or displayed to the user.

### Why It's a Problem

- **Cognitive Overhead:** Developers waste precious time reading, analyzing, and trying to understand code that doesn't actually do anything. It creates the illusion of complexity where none exists.
- **Refactoring Friction:** When a developer tries to update a data model or refactor a system, they are forced to also update the dead code, completely unaware that their effort is being wasted on unused logic.
- **Unnecessary Bloat:** In compiled or bundled languages (like JavaScript sent to a browser), dead code needlessly increases the file size, negatively impacting download speeds and performance.

### How to Fix It

- **Delete It Without Mercy:** Just like Patch Artifacts, rely on your version control system (Git). If the code is dead, remove it entirely. You can always get it back if you need it later.
- **Leverage IDEs and Linters:** Most modern IDEs (like VS Code or IntelliJ) will visually "gray out" unreachable code or unused variables. Strict linters (like ESLint) can be configured to throw errors if unused code is detected, preventing it from ever being committed.
- **Use Code Coverage Tools:** Run your test suite with a code coverage tool. If a massive block of business logic reports 0% coverage, investigate to see if it's dead code that can safely be removed.

## 12. Inconsistent Error Handling

**Inconsistent Error Handling** happens when a codebase lacks a unified strategy for dealing with failures. Instead of a predictable pattern, different parts of the application throw, return, or ignore errors using completely different paradigms.

### Common Characteristics

- **The Mixed Bag:** One function returns `null` on failure, another returns an object like `{ success: false, message: '...' }`, a third function throws an `Exception`, and a fourth returns a `-1` error code.
- **Swallowed Exceptions:** Empty `catch (e) {}` blocks that silently catch errors and do nothing with them, making it impossible to know a failure occurred.
- **Scattered Try-Catches:** Every single function in a file is wrapped in its own redundant `try/catch` block instead of letting errors bubble up to a centralized handler.
- **Generic Errors:** Throwing base `Error("Something went wrong")` instead of specific, meaningful custom error types like `UserNotFoundError` or `DatabaseTimeoutError`.

### Why It's a Problem

- **Zero Predictability:** A developer calling a function has no idea how to safely use it without reading the source code. Do they need to wrap it in a `try/catch`? Do they need to check if the result is `null`?
- **Silent Failures:** When errors are swallowed or inconsistently caught, applications fail in mysterious ways, leaving users confused and giving developers no logs to debug.
- **Data Corruption:** If an error isn't handled correctly and the application continues executing, it can lead to corrupted databases or unauthorized actions.

### How to Fix It

The solution is to agree on a global, application-wide standard for how errors should behave.

- **Establish a Paradigm:** Decide as a team: *Are we throwing exceptions, or are we returning error objects/monads (like Rust's Result type)?* Stick to that paradigm everywhere.
- **Use Custom Error Classes:** Create explicit classes for different failure types (e.g., `ValidationError`, `AuthenticationError`). This allows your code to react differently depending on *what* went wrong.
- **Centralize Handling:** Instead of catching errors in every single function, let them bubble up to a global error handler. In web frameworks (like Express or React), you can create middleware or Error Boundaries to catch unhandled errors, log them securely, and show a friendly message to the user.
- **Fail Fast and Loud:** Never swallow errors silently. If something unexpected happens, the code should fail noticeably so it can be fixed.