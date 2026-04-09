# Matching Engine

## Overview

Minimal C++ matching engine project scaffold with CMake, GoogleTest, and Google Benchmark integration.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run tests

```bash
ctest --test-dir build --output-on-failure
```

## Run benchmarks

Benchmark targets are built via CMake with Google Benchmark available in the project.  
Add and run benchmark executables as they are introduced.

## Architecture

## Design decisions

## Benchmarks

## Roadmap

## References
