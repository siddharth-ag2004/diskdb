# Disk-Based Relational Database Engine (SimpleRA)

A robust, disk-based relational database management system built in C++. This project is designed to handle large datasets that exceed main memory capacity by implementing sophisticated external memory algorithms, custom page-based storage, and an advanced buffer manager.

## Key Features

- **Page-Based Storage Management:** Custom implementation of disk paging to efficiently read/write data blocks, bypassing standard OS-level caching constraints.
- **Buffer Manager:** Intelligent in-memory buffer pool that minimizes disk I/O by caching frequently accessed pages and employing page replacement policies.
- **SQL-Like Query Processing:** Supports core relational algebra operations including:
  - **External SORT:** Capable of sorting datasets significantly larger than the available RAM using external merge sort.
  - **HASH JOIN:** Implements block nested-loop and hash joins for efficiently combining large tables.
  - **GROUP BY:** Aggregation operations processed securely over external memory.
- **Graph Processing Extension:** Extended functionality to support large-scale graph datasets, allowing for:
  - Path queries and traversal.
  - Node and edge constraint filtering.
  - Processing graphs that cannot fit into main memory.

## Setup and Compilation

### Prerequisites
- **C++ Compiler** (g++ or clang++ with C++17 support recommended)
- **Make**

### Building the Project

Navigate to the source directory and compile the database engine using `make`:

```bash
cd src
make clean
make
```

### Running the Database Engine

After successful compilation, an executable named `server` will be generated in the `src` directory. To start the engine:

```bash
./server
```

## Documentation & Usage

The database uses a custom query language (SimpleRA).
For detailed explanations of the grammar, sample queries, and usage examples, please refer to the following documents:
- [Overview](docs/Overview.md)
- [Grammar](Grammar.md)
