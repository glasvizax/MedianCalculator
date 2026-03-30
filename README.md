# MedianCalculator

## 1. Description

**MedianCalculator** is a C++ application for processing multiple CSV files containing time-series data and computing median deviations over a merged dataset.

The application:

* Parses multiple CSV files with specified columns
* Merges and sorts data by timestamp
* Computes running median values
* Outputs only elements where the median value changes beyond a specified threshold

The processing pipeline is optimized for performance and uses:

* efficient sorting and merging algorithms
* lock-based producer-consumer pipeline for output
* fast numeric formatting (`std::to_chars`)

---

## 2. Requirements

### System requirements

* C++23 compatible compiler:

  * GCC 12+
  * Clang 15+
  * MSVC 2022+

* CMake ≥ 3.23

### Dependencies

* **Boost** (required, not fetched automatically)

  * Component: `program_options`

* Fetched automatically via CMake:

  * [spdlog](https://github.com/gabime/spdlog)
  * [toml++](https://github.com/marzer/tomlplusplus)
  * [GoogleTest](https://github.com/google/googletest) (optional, for tests)

---

## 3. Build Instructions


```bash
cmake -B build -S . [-DCMAKE_BUILD_TYPE=YourBuildType]
```

### Step 2: Build

```bash
cmake --build build [--config YourConfigType]
```

### Optional: Enable/disable features

```bash
- USE_PCH=ON/OFF              	# Precompiled headers
- TEST_MedianCalculator=ON/OFF # Enable or disable tests
- TESTS_DROP_LOGGING=ON/OFF 	# Enable or disable program logging in tests
```

---

## 4. Running the Application

### Basic usage

```bash
./Main -cfg YourConfig.toml
```

### Parameters

| Parameter          | Description                     |
| ------------------ | ------------------------------- |
| `-cfg / -config`   | Path to toml configuration file |

---

## 5. Configuration Format

The application uses a **TOML configuration file**.

### Example (`config.toml`)

```toml
input_dir = "./data"
file_pattern = ["basic", "data"]
output = "./output/"
```

### Parameters

| Field          | Type   | Description                      |
| -------------- | ------ | -------------------------------- |
| `input_dir`    | string | Directory containing CSV files   |
| `file_pattern` | array  | File matching pattern (optional) |
| `output`       | string | Output CSV path  (optional)      |

---

## 6. Examples

### Input CSV

```csv
receive_ts;price;
1000;10.5
2000;11.0
3000;10.8
```

Only rows where **median changes significantly** are written.

---

## Notes

* Logging is enabled in non-release builds
* Algorithms are optimized for large datasets
* Empty datasets are handled safely
* Output writing is asynchronous

---

## Tests

To build and run tests:

```bash
cmake -B build -DTEST_MedianCalculator=ON
cmake --build build
ctest --test-dir build [-C YourConfigType]
```

---
