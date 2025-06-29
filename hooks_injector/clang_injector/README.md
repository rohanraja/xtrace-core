# xTrace Clang Injector

A powerful C++ code injection tool built with Clang LibTooling for the xTrace debugging system. This tool provides robust AST-based analysis and code injection capabilities, offering superior accuracy compared to tree-sitter-based approaches.

## Features

- **AST-based Analysis**: Uses Clang's full semantic understanding of C++ code
- **Method Entry Logging**: Automatically injects xTrace method entry calls
- **Parameter Logging**: Captures function parameters with type-aware serialization
- **Robust Parsing**: Handles complex C++ constructs, templates, and macros
- **Configurable**: Flexible include/exclude patterns for method filtering
- **Standards Compliant**: Works with modern C++ standards (C++11, C++14, C++17, C++20)

## Quick Start

### 1. Setup (First Time)

```bash
# Complete setup - installs dependencies, configures, and builds
npm run setup
```

### 2. Basic Usage

```bash
# Process a C++ file
./build/xtrace-clang-injector input.cpp -- -std=c++17

# Process from stdin
echo 'int main() { return 0; }' | ./build/xtrace-clang-injector /dev/stdin --

# With verbose output
./build/xtrace-clang-injector --verbose input.cpp -- -std=c++17
```

### 3. VS Code Integration

Use the provided VS Code tasks:

- **Ctrl+Shift+P** → "Tasks: Run Task"
- Select "Build: Compile" to build the project
- Select "Run: Process Current File" to process the currently open C++ file
- Select "Test: Run Sample Test" to run automated tests

## Installation

### Prerequisites

The tool requires LLVM/Clang development libraries. The setup script will attempt to install these automatically.

#### Ubuntu/Debian
```bash
sudo apt-get install clang-15 llvm-15-dev libclang-15-dev cmake build-essential
```

#### macOS
```bash
brew install llvm cmake
```

#### Manual Setup Steps

```bash
# 1. Install dependencies
npm run install-deps

# 2. Configure build
npm run configure

# 3. Build
npm run build

# 4. Test
npm run test
```

## Usage Examples

### Basic Function Injection

**Input:**
```cpp
int add(int a, int b) {
    return a + b;
}
```

**Output:**
```cpp
#include "third_party/xtrace/xtrace.h"
#include "base/strings/to_string.h"
int add(int a, int b) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter("input.cpp", "add", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "a", base::ToString(a));
  xtrace->LocalVarUpdate(xtrace_mrid, "b", base::ToString(b));
    return a + b;
}
```

### Class Method Injection

**Input:**
```cpp
class Calculator {
public:
    int multiply(int x, int y) {
        int result = x * y;
        return result;
    }
};
```

**Output:**
```cpp
#include "third_party/xtrace/xtrace.h"
#include "base/strings/to_string.h"
class Calculator {
public:
    int multiply(int x, int y) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter("input.cpp", "Calculator::multiply", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "x", base::ToString(x));
  xtrace->LocalVarUpdate(xtrace_mrid, "y", base::ToString(y));
        int result = x * y;
        return result;
    }
};
```

## Architecture

### Core Components

1. **XTraceInjector**: Main AST consumer that orchestrates the injection process
2. **MethodVisitor**: Traverses the AST to discover function declarations
3. **CodeRewriter**: Handles the actual code modification and injection
4. **Configuration**: Manages include/exclude patterns and injection rules

### Project Structure

```
hooks_injector/clang_injector/
├── CMakeLists.txt          # Build configuration
├── package.json            # NPM scripts and metadata
├── README.md              # This file
├── .vscode/
│   └── tasks.json         # VS Code task definitions
├── include/               # Header files
│   ├── XTraceInjector.h   # Main injector class
│   ├── MethodVisitor.h    # AST visitor for methods
│   └── CodeRewriter.h     # Code modification utilities
├── src/                   # Source files
│   ├── main.cpp           # CLI entry point
│   ├── XTraceInjector.cpp # Main injector implementation
│   ├── MethodVisitor.cpp  # AST traversal logic
│   └── CodeRewriter.cpp   # Code injection logic
└── scripts/               # Build and setup scripts
    ├── setup.sh           # Complete setup
    ├── install-deps.sh    # Dependency installation
    ├── configure.sh       # CMake configuration
    ├── build.sh           # Build process
    ├── test.sh            # Testing
    └── clean.sh           # Cleanup
```

## Configuration

The tool uses compile-time configuration for method filtering:

### Include/Exclude Patterns

```cpp
// In src/MethodVisitor.cpp
static const std::set<std::string> methods_to_exclude = {
    "operator",    // Operator overloads
    "~",          // Destructors
    "__",         // Internal methods
    "TEST_",      // Test methods
    "EXPECT_",    // Test assertions
    "ASSERT_"     // Test assertions
};

static const std::set<std::string> methods_which_split_run = {
    "main",       // Main entry points
    "OnStart",    // Initialization methods
    "Initialize"  // Setup methods
};
```

### Runtime Configuration

The tool supports command-line options:

```bash
./build/xtrace-clang-injector [options] <input-file> -- [clang-options]

Options:
  --verbose                Enable verbose output
  --output <file>         Output file (default: stdout)
  --help                  Show help message
```

## Advanced Usage

### Custom Clang Options

Pass any Clang compilation flags after the `--` separator:

```bash
# With custom include paths
./build/xtrace-clang-injector input.cpp -- -I/custom/include -std=c++20

# With preprocessor definitions
./build/xtrace-clang-injector input.cpp -- -DDEBUG=1 -DFEATURE_ENABLED

# With specific target
./build/xtrace-clang-injector input.cpp -- -target x86_64-linux-gnu
```

### Integration with Build Systems

#### CMake Integration

```cmake
# In your CMakeLists.txt
find_program(XTRACE_INJECTOR xtrace-clang-injector)

function(add_xtrace_injection target source_file)
    add_custom_command(
        OUTPUT ${source_file}.injected
        COMMAND ${XTRACE_INJECTOR} ${source_file} -- -std=c++17 > ${source_file}.injected
        DEPENDS ${source_file}
        COMMENT "Injecting xTrace into ${source_file}"
    )
endfunction()
```

#### Makefile Integration

```makefile
%.injected.cpp: %.cpp
	./path/to/xtrace-clang-injector $< -- -std=c++17 > $@
```

## Troubleshooting

### Common Issues

1. **LLVM/Clang not found**
   ```bash
   # Make sure LLVM is installed and in PATH
   llvm-config --version
   
   # If not found, run dependency installation
   npm run install-deps
   ```

2. **Build failures**
   ```bash
   # Clean and reconfigure
   npm run clean
   npm run configure
   npm run build
   ```

3. **Missing headers during injection**
   ```bash
   # Make sure your code can compile with standard Clang
   clang++ -std=c++17 -fsyntax-only your_file.cpp
   
   # Pass the same flags to the injector
   ./build/xtrace-clang-injector your_file.cpp -- -std=c++17 -I/your/includes
   ```

### Debug Mode

Build in debug mode for troubleshooting:

```bash
# Configure for debug
cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make

# Or use VS Code task: "Debug: Build Debug Version"
```

## Performance

The Clang-based approach offers several performance advantages:

- **Accuracy**: No false positives from syntax errors
- **Speed**: Efficient AST traversal
- **Memory**: Reasonable memory usage for large files
- **Scalability**: Handles complex C++ codebases

### Benchmarks

Typical performance on a modern system:

- Small files (< 1K LOC): < 100ms
- Medium files (1K-10K LOC): 100ms-1s
- Large files (10K+ LOC): 1-10s

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Run the test suite: `npm run test`
6. Submit a pull request

## License

MIT License - see LICENSE file for details.

## Comparison with Tree-sitter Approach

| Feature | Clang LibTooling | Tree-sitter |
|---------|-----------------|-------------|
| Accuracy | ✅ Full semantic analysis | ⚠️ Syntax-only parsing |
| C++ Support | ✅ Complete C++ standard | ⚠️ Limited complex constructs |
| Templates | ✅ Full template support | ❌ Basic support only |
| Macros | ✅ Macro expansion | ❌ No expansion |
| Error Handling | ✅ Compiler-grade errors | ⚠️ Basic syntax errors |
| Performance | ✅ Fast compilation | ✅ Very fast parsing |
| Dependencies | ⚠️ Requires LLVM/Clang | ✅ Standalone |
| Maintenance | ✅ Follows Clang updates | ⚠️ Manual grammar updates |

The Clang LibTooling approach provides superior accuracy and robustness for production C++ codebases, especially those using modern C++ features.
