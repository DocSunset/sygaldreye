Be brief. Less is more. Say less.

Write less code. The best code is no code. Every line has a cost.

# Project Standard

- The basic unit of implementation is a software component.
- A software component is embodied by its documents: design documentation, production source code, automated tests, and build automation scripts.
    - Component names use `lower_snake_case`.
    - Design documents are called `component_name.design.md`
    - Source code documents use the extensions appropriate for the source code (e.g. `.hpp`, `.cpp`)
    - Automated tests are called `component_name.test.xxx` using the extension appropriate for the source code
    - We use cmake for build automation
    - All documents should be placed in a directory whose name is the name of the component
- Design documentation must document the following design features:
    - Ports
        - Inputs: Places where influence comes into the component that semantically belong to the component and do not depend on how other components behave
        - Outputs: Places where influence flows from the component that semantically belong to the component and do not depend on how other components behave
        - Sources: Places where influence comes into the component that depend on the behavior of another component; they are coupled inputs
        - Destinations: Places where influence flows from the component that must meet the needs of another component; they are coupled outputs
        - Temportal couplings: Places where the behavior of the component is coupled to another component's events happening in a certain order
        - Intended seams: Places where the behavior of the component is intended to be extended without modifying the component's own source code, e.g. for testing or derived components
    - Requirements (e.g. functional and non-functional and so on)
    - Allowed dependencies: an explicit list of names of components this one is allowed to depend on
    - Owning package: the name of the package to which the component belongs
- Packages are groups of components
- A package should have a `package_name.design.md` document that lists the other packages that components in the package are allowed to depend on.

# Tooling

- Prefer Nix as package manager for bringing in external dependencies
- `sh/build.sh [--clean]`: build the application
- `sh/format.sh [--check]`: auto-formatter
- `sh/lint.sh`: static analyzers
- `sh/test.sh`: automated tests (uses ctest; test binaries use Catch2 v3)
- `sh/run.sh [args]`: run the application interactively

## Testing

- Use **Catch2 v3** (`Catch2::Catch2WithMain`) for all component test executables.
- Test files are named `component_name.test.cpp` and live in the component directory.
- Register tests with `catch_discover_tests()` in CMake so ctest picks them up automatically.

# Goal

TODO

# Intended Architecture

TODO

Architectural evolution should be documented in `adr.md`.

# Workflow

Work items are located in the `kanban` directory.

# Remember

Be brief. Less is more. Say less.

Write less code. The best code is no code. Every line has a cost.
