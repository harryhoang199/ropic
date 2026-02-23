# Railway Oriented Programming in C++ (ropic)

A C++20 coroutine-based implementation of Railway Oriented Programming (ROP) for elegant error handling.

## Why Ropic?

### Benefits

- Provides a mechanism that effectively supersedes traditional `try-catch` blocks. Errors flow through the call chain automatically via `co_await`, eliminating the need for manual exception handling infrastructure.

- Guarantees that functions do not throw exceptions (`noexcept`), while providing deterministic visibility into whether a function propagates errors based on its signature.

- Promotes clean, concise code, eliminating the boilerplate and nesting associated with excessive `if-else` checks or `try-catch` blocks.

### Function Contract Examples

A function's error-handling behavior is immediately visible from its signature:

**Error-propagating & Non-propagating functions**:

```cpp
// Error-propagating function: returns `Either`, marked `noexcept`
// This is a wrapper of traditional try-catch.
// It may produce errors but will never throw
ropic::Either<double, Error> parseDouble(const std::string& s) noexcept {
    // Implementation uses co_return for both success and error paths
  try {
    co_return std::stod(str);
  }
  catch (...)
  {
    co_return Error{"Cannot parse '" + str + "' to double"};
  }
}

// Non-propagating function: returns non-`Either`, marked `noexcept`
// This function neither throws nor propagates errors
double clamp(double value, double min, double max) noexcept {
    return std::max(min, std::min(value, max));
}
```

**Comparison with traditional approaches:**

```cpp
// ropic: Flat, composable, explicit error handling
ropic::Either<double, Error> computeRatioCoawait(const std::string& a, const std::string& b) noexcept {
    double x = co_await parseDoubleCoawait(a);  // Error auto-propagates
    double y = co_await parseDoubleCoawait(b);  // Error auto-propagates
    double z = co_await divideCoawait(x, y);    // Error auto-propagates
    co_return z;
}

// try-catch: Hidden control flow, nested blocks
double computeRatioTryCatch(const std::string& a, const std::string& b) {
    try {
        double x = parseDoubleTryCatch(a);
        double y = parseDoubleTryCatch(b);
        if (y == 0) throw std::runtime_error("Division by zero");
        return x / y;
    } catch (const std::exception& e) {
        // Error handling here, but caller doesn't know this can throw
        throw;
    }
}

// if-else: Verbose, repetitive error checking
std::optional<double> computeRatioIfElse(
    const std::string& a, const std::string& b, std::string& errorOut) noexcept {
    std::optional<double> x = parseDoubleIfElse(a);
    if(!x.has_value()) {
        errorOut = "Invalid first number";
        return std::nullopt;
    }

    std::optional<double> b = parseDoubleIfElse(b);
    if(!y.has_value()) {
        errorOut = "Invalid second number";
        return std::nullopt;
    }

    if (y.value() == 0) {
        errorOut = "Division by zero";
        return std::nullopt;
    }

    return x.value() / y.value();
}
```

## More Usage Examples

### Basic Either Usage

```cpp
// Function returning Either<VALUE, Error> with data or error
ropic::Either<int, Error> divide(int a, int b) noexcept {
    // co_return an `int` or an `Error`
    if (b == 0) {
        co_return Error{"Division by zero"};
    }
    co_return a / b;
}

// Using the result - always check state() before accessing error()/value()
auto result = divide(10, 2);
assert(result.state() == ropic::CoroState::DONE);  // For synchronous coroutines, always true
if (auto err = result.error()) {
    std::cerr << "Error: " << err->message() << '\n';
} else if (auto val = result.value()) {
    std::cout << "Result: " << *val << '\n';
}
```

### Automatic Error Propagation with co_await

```cpp
ropic::Either<double, Error> parseDouble(const std::string& s) noexcept;
ropic::Either<double, Error> divide(double numerator, double denominator) noexcept;

ropic::Either<double, Error> divideStr(const std::string& numStr, const std::string& denStr) noexcept {
    // co_await automatically extracts data or propagates errors
    double x = co_await parseDouble(numStr);

    // Remove co_await to catch and handle error right here
    auto y = parseDouble(denStr);
    assert(y.state() == ropic::CoroState::DONE);  // Synchronous coroutine is always done
    if (auto err = y.error())
        co_return *err;

    double result = co_await divide(x, *(y.value()));
    co_return result;
}
```

### Chaining Multiple Operations

```cpp
ropic::Either<std::vector<int>, Error> getWeights() noexcept;

ropic::Either<double, Error> compute(const std::string& a, const std::string& b) noexcept {
    // Use `co_await` with the `Either` template with a different `VALUE` type
    // (the `ERROR` types must be the same)
    std::vector<int> weights = co_await getWeights();
    if (weights.size() < 2)
        co_return Error{"Need at least 2 weights"};

    double x = co_await parseDouble(a);
    double y = co_await parseDouble(b);
    co_return x * weights[0] + y * weights[1];
}
```

### Void Specialization for Error-Only Operations

```cpp
ropic::Either<ropic::Void, Error> saveConfig(const Config& cfg) noexcept {
    if (!validateConfig(cfg)) {
        co_return Error{"Invalid configuration"};
    }
    writeToFile(cfg);
    co_return ropic::OK;  // Success, no data (use OK or VOID constant)
}

auto result = saveConfig(config);
assert(result.state() == ropic::CoroState::DONE);  // Always check state() before accessing error()/value()
if (auto err = result.error()) {
    std::cerr << "Save failed: " << err->message() << '\n';
} else {
    std::cout << "Saved successfully\n";
}
```

### Integrating with Other Coroutines

Either coroutines can seamlessly integrate with other coroutine types in both directions.

**Using non-Either awaitables inside Either coroutines (Safe API):**

Either provides a Safe Awaitable API using `ropic::ResumeSource` for thread-safe integration with external async operations — no mutex required. An awaitable is "safe" when its `await_suspend` accepts a `ropic::ResumeSource` instead of a raw `std::coroutine_handle<>`. The library automatically manages coroutine lifecycle and synchronization.

```cpp
// A safe awaitable that simulates async data fetching
// Key: await_suspend receives ResumeSource instead of coroutine_handle
struct SafeAsyncFetch {
    std::string data;
    explicit SafeAsyncFetch(std::string data) : data(std::move(data)) {}

    bool await_ready() { return false; }
    void await_suspend(ropic::ResumeSource resumeSrc) {
        // No mutex needed! ResumeSource handles thread-safe resume signaling
        std::thread([src = std::move(resumeSrc)]() mutable {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            src.requestResume();  // Signal readiness — safe from any thread
        }).detach();
    }
    std::string await_resume() { return std::move(data); }
};

// Either coroutine using safe awaitables
ropic::Either<double, Error> asyncDivide(std::string numStr, std::string denStr) noexcept {
    std::string fetchedNum = co_await SafeAsyncFetch{std::move(numStr)};
    std::string fetchedDen = co_await SafeAsyncFetch{std::move(denStr)};
    double result = co_await divideStr(fetchedNum, fetchedDen);
    co_return result;
}

// Poll for completion — no mutex required thanks to Safe API
auto task = asyncDivide("42", "7");
while (true) {
    switch (task.state()) {
    case ropic::CoroState::DONE:
        goto done;          // Task completed
    case ropic::CoroState::READY:
        task.resume();      // Resume the coroutine — ready to continue
        break;
    case ropic::CoroState::PENDING:
        break;              // Still waiting for async operation
    }
}
done:
if (auto err = task.error())
    std::cerr << "Error: " << err->message() << '\n';
else
    std::cout << "Result: " << *task.value() << '\n';
```

**Using non-Either awaitables inside Either coroutines (manual control):**

If you prefer full control over coroutine resumption without using the Safe API, you can use standard `std::coroutine_handle<>` directly. This requires manual thread synchronization (e.g., mutex) to safely resume coroutines from other threads.

```cpp
// Example of using standard awaitables with manual locking
// This requires a mutex and checking state() before accessing error()/value()

std::mutex mtx;

struct AsyncFetch {
    std::string data;
    explicit AsyncFetch(std::string data) : data(std::move(data)) {}

    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        std::thread([h] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::lock_guard<std::mutex> lock(mtx);  // Manual locking required
            h.resume();
        }).detach();
    }
    std::string await_resume() { return std::move(data); }
};

ropic::Either<double, Error> asyncDivide(std::string numStr, std::string denStr) noexcept {
    std::string fetchedNum = co_await AsyncFetch{std::move(numStr)};
    std::string fetchedDen = co_await AsyncFetch{std::move(denStr)};
    double result = co_await divideStr(fetchedNum, fetchedDen);
    co_return result;
}

// Poll for completion — mutex required for thread safety
auto task = asyncDivide("42", "7");
while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> lock(mtx);
    if (task.state() == ropic::CoroState::DONE)
        break;
}
if (auto err = task.error())
    std::cerr << "Error: " << err->message() << '\n';
else
    std::cout << "Result: " << *task.value() << '\n';
```

**Using Either coroutines inside non-Either coroutines:**

When `co_await`-ing an Either from a non-Either coroutine (like Task or Generator), the Either object itself is returned (not unwrapped), allowing manual error handling.

```cpp
template<typename T>
struct Task {
    // ... promise_type and coroutine infrastructure ...
    T run();  // Resumes and returns result
};

// Task coroutine that calls Either-returning functions
Task<double> computeInTask(std::string a, std::string b) {
    // co_await on Either returns the Either object (via InteropAwaiter)
    auto result1 = co_await divideStr(a, b);
    auto result2 = co_await divideStr(b, a);

    // Always check state() before accessing error()/value()
    if (result1.state() != ropic::CoroState::DONE 
     || result2.state() != ropic::CoroState::DONE)
        co_return -1.0;  // Error sentinel

    // Manual error checking required in non-Either coroutines
    if (result1.error() || result2.error())
        co_return -1.0;  // Error sentinel

    co_return result1.value().value() * result2.value().value();
}

// Run the task
auto task = computeInTask("10", "2");
double result = task.run();
```

## Build Requirements

- C++20 compiler with coroutine support
- CMake 3.28 or higher
- Supported compilers: MSVC, GCC, Clang

## Installation

### Option 1: System-wide Installation

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Install (may require admin/sudo)
cmake --install build --prefix /usr/local  # Linux/macOS
cmake --install build --prefix "C:/Program Files/ropic"  # Windows
```

### Option 2: CMake FetchContent (Recommended)

Add this to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
  ropic
  GIT_REPOSITORY https://github.com/harryhoang199/ropic.git
  GIT_TAG        main
)

FetchContent_MakeAvailable(ropic)

# Link to your target
target_link_libraries(your_target PRIVATE ropic::ropic)
```

### Option 3: Add as Subdirectory

Clone or copy the repository into your project:

```bash
git clone https://github.com/harryhoang199/ropic.git external/ropic
```

Then in your `CMakeLists.txt`:

```cmake
add_subdirectory(external/ropic)
target_link_libraries(your_target PRIVATE ropic::ropic)
```

## Using the Installed Library

After installation, use `find_package` in your project:

```cmake
cmake_minimum_required(VERSION 3.28)
project(my_project LANGUAGES CXX)

find_package(ropic REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE ropic::ropic)
```

Then include the headers in your code:

```cpp
#include <ropic>               // Main header
#include <ropic/version.hpp>   // Version info (optional)

int main() {
    // Use ropic::Either, ropic::Error, etc.
    std::cout << "Using ropic v" << ropic::VERSION << "\n";
}
```

## CMake Options

| Option                   | Default | Description                             |
| ------------------------ | ------- | --------------------------------------- |
| `ROPIC_BUILD_EXAMPLES`   | `OFF`   | Build example executable                |
| `ROPIC_BUILD_TESTING`    | `OFF`   | Build tests (requires GTest)            |
| `ROPIC_BUILD_BENCHMARKS` | `OFF`   | Build tests (requires Google Benchmark) |

Example:

```bash
cmake -B build -DROPIC_BUILD_EXAMPLES=ON -DROPIC_BUILD_TESTING=ON
```

## Building Examples and Tests

```bash
# Configure with examples and tests
cmake -B build

# Build
cmake --build build --config Debug

# Run example
./build/bin/Debug/ropic-examples      # Linux/macOS
.\build\bin\Debug\ropic-examples.exe  # Windows

# Run tests
./build/bin/Debug/either-tests       # Linux/macOS
.\build\bin\Debug\either-tests.exe   # Windows
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
