
# SimpleRA Phase 2

The detailed implementation logic, block access costs, and error handling for the implemented commands: SETBUFFER, SORT, HASH JOIN, and GROUP BY are given below:

---

## Implementation

### 1. SETBUFFER K

#### Logic
The SETBUFFER command modifies the global session limit for the number of blocks (pages) that can be held in main memory simultaneously. 
*   Syntactic Parsing: The command checks if the token count is exactly 2. The string corresponding to K is parsed into an integer (parsedQuery.setBufferSize).
*   Execution: The global variable BLOCK_COUNT is updated to the parsed value. 
*   Buffer Manager Adjustment: The BufferManager enforces this constraint dynamically. Within BufferManager::insertIntoPool(), a while (this->pages.size() >= BLOCK_COUNT) loop evicts the oldest pages (FIFO policy via pop_front()) before reading a new block. Thus, reducing the buffer size takes effect immediately on subsequent reads.

#### Block Access
*   I/O Cost: $O(1)$ block accesses. The command itself only modifies a global integer variable. No direct disk reads or writes happen during its execution.
*   Memory Evictions: If the buffer size is reduced, subsequent data reads might force eviction of existing blocks, but SETBUFFER explicitly performs no disk I/O.

#### Error Handling
*   Syntax Errors: Thrown if the number of arguments is incorrect or if <K> cannot be cast to an integer.
*   Semantic Errors: Thrown if <K> is not within the specified constraints ($2 \le K \le 10$).

---

### 2. SORT

#### Logic
The SORT command implements the K-way Multi-Phase Merge Sort algorithm to sort tables in-place (updating temp pages, original .csv remains unaltered). It supports multiple sort keys, ascending/descending priorities, and the TOP X / BOTTOM Y clauses.
*   Partial Sorting (TOP/BOTTOM):
    If TOP X or BOTTOM Y is specified, the input file is scanned once to slice it into three temporary unmerged tables: Top, Middle, and Bottom. The code limits TOP and BOTTOM bounds to prevent overlaps. The Top and Bottom tables are then sorted recursively using the standard Multi-Phase Merge Sort logic, while the middle table is left unsorted. Finally, the three tables are sequentially appended to form the final result.
*   Full Sorting (K-way Multi-Phase Merge Sort):
    *   Phase 1 (Run Generation): The table is read into memory in chunks of BLOCK_COUNT blocks. Each chunk is sorted in-memory using a standard Merge Sort (mergeSortRows()) and written to disk as an initial run.
    *   Phase 2 (Multi-Pass Merging): The initial runs are merged in groups of mergeDegree = BLOCK_COUNT - 1. A custom HeapCompare struct and a Min-Heap (std::priority_queue) are used to hold the smallest element from each run and process the merge efficiently. The merged results are written block-by-block sequentially. This is repeated pass-by-pass until a single run remains. 

#### Block Access
Let $B$ be the total blocks in the table, and $K$ be BLOCK_COUNT.
*   Run Generation: $B$ reads and $B$ writes. (Cost = $2B$)
*   Merging Passes: The number of runs generated initially is $\lceil B / K \rceil$. Each merge pass merges $K-1$ runs. The number of merge passes is $\lceil \log_{K-1}(B/K) \rceil$. In each pass, every block is read once and written once. 
*   Total Block Access: $2B \times (1 + \lceil \log_{K-1}(B/K) \rceil)$
*   *For TOP/BOTTOM*: An additional $2B$ accesses exist for the slicing and appending phases, plus the internal sort cost over smaller sliced tables.

#### Error Handling
*   Syntax Errors: Handles missing keywords (BY, IN), malformed attributes, and non-integer values for TOP and BOTTOM.
*   Semantic Errors: Validates that the table exists and all specified sort-key columns exist within the table.

---

### 3. HASH JOIN

#### Logic
The HASH JOIN implements a Hash Join with an optimized base case (In-Memory Probe). It evaluates arithmetic conditions (A.a + B.b == X), WHERE selections, and projections.
*   Base Case (In-Memory Probing): 
    If $\min(B_1, B_2) \le BLOCK\_COUNT - 2$, the algorithm reads the smaller table entirely into memory using an unordered_multimap for hash lookups. It then streams the larger table block-by-block, looking for join matches (performProbing). Matching rows are filtered through the WHERE condition (if provided) and specified PROJECT columns are sliced before writing to the output buffer.
*   Partitioning Phase:
    If both tables are larger than the available buffer minus two, the code partitions them. It generates $M = max(1, BLOCK\_COUNT - 1)$ partitions by hashing the join keys using ((key % M) + M) % M. Each record is sent to its corresponding partition buffer and flushed to disk.
*   Probing Phase:
    After partition generation, the algorithm pairs corresponding partitions ($P1_i, P2_i$) and recursively calls the performProbing logic (since these partitions are now smaller and likely fit into the buffer limit). Temporary partition tables are then deleted.

#### Block Access
Let $B_R$ and $B_S$ be the blocks in table 1 and table 2. $B_{out}$ is the number of blocks generated.
*   If tables fit in memory (Base case): 
    *   Read smaller table: $B_{small}$
    *   Read larger table: $B_{large}$
    *   Total I/O: $B_R + B_S + B_{out}$ writes.
*   If tables are partitioned:
    *   Read both tables and write out $M$ partitions: $2 \times (B_R + B_S)$ accesses.
    *   Read partitions during probe phase: $B_R + B_S$ accesses.
    *   Total I/O: $3 \times (B_R + B_S) + B_{out}$ writes.

#### Error Handling
*   Syntax Errors: Thrown for malformed queries, invalid arithmetic operators, unsupported comparison operators, missing conditions, or poorly formatted projections.
*   Semantic Errors: Thrown if the required tables don’t exist, the target result table *already* exists, or if columns referenced in the ON, WHERE, or PROJECT clauses do not exist in their corresponding tables. It gracefully discards empty result tables to avoid memory leaks if no rows match.

---

### 4. GROUP BY

#### Logic
The GROUP BY command handles aggregate grouping, a HAVING clause, and multiple result tables generated concurrently. It achieves this via a sort-based grouping mechanism.
*   Execution Strategy: 
    The command processes each grouping attribute individually. For a grouping attribute, it first sorts the original table by that attribute using externalSortCreateNewTable (into a temporary table). This guarantees that all rows belonging to the same group are positioned contiguously.
*   Streaming & Aggregation:
    It linearly scans the sorted temporary table using a Cursor. A custom struct, AggState, accumulates statistics (SUM, COUNT, MIN, MAX) for the HAVING left-hand expression, the HAVING right-hand expression (if not a constant), and the RETURN aggregate.
*   Filtering & Output:
    When the grouping attribute value changes (indicating a group transition), the algorithm evaluates the HAVING condition using evaluateBinOp. If the condition holds true, a row containing the grouping attribute value and the resulting aggregate value is generated and pushed to the result buffer. Once verified, the state is reset for the next group.
*   Cleanup:
    The result block is flushed to disk, and the sorted temporary table is deleted from the TableCatalogue to prevent accumulation of temp files. 

#### Block Access
Let $B$ be the blocks in the source table. For each grouping attribute out of $G$ attributes:
*   Sorting Phase: The system generates a sorted temp table. Block access is equivalent to $2B \times (1 + \lceil \log_{K-1}(B/K) \rceil)$.
*   Scanning Phase: The sorted table is read sequentially once ($B$ reads).
*   Writing Results: Let the returned rows span $B_{res\_i}$ blocks. Output requires $B_{res\_i}$ writes.
*   Total Block Access: $\sum_{i=1}^{G} \Big( Cost(Sort) + B_{temp} + B_{res\_i} \Big)$

#### Error Handling
*   Syntax Errors: Thrown for invalid aggregate functions (must be MAX, MIN, COUNT, SUM, AVG), missing clauses (BY, FROM, HAVING, RETURN), or invalid arithmetic operators.
*   Semantic Errors: 
    *   Relation doesn't exist.
    *   Group-by, Having, or Return attributes do not exist in the table (ignores * for COUNT(*)).
    *   The count of resultant tables does not match the count of grouping attributes and return expressions.
    *   Resultant tables already exist in the database.
    *   Empty resultant tables (where no groups satisfy HAVING) are correctly bypassed and not registered in the catalogue.

## Assumptions

### General & Buffer Management (SETBUFFER)
*   Lazy Eviction on Buffer Resize: As clarified in the TA doubt document, when SETBUFFER K is executed to reduce the buffer size, excess pages currently in main memory are not immediately dropped. Instead, the BufferManager dynamically adjusts and evicts the oldest pages (FIFO) during subsequent reads (insertIntoPool).
*   Integer Data Types: The database strictly processes and evaluates integer values. All arithmetic, comparisons, aggregates, and sorting logic assume integer bounds. 

### SORT Command
*   In-Place Temporary Sorting: Sorting a table modifies the active, loaded instance of the table residing in the temporary pages (e.g., /data/temp/). The original .csv file in the /data/ directory remains completely untouched unless an explicit EXPORT command is subsequently called.
*   Non-Overlapping TOP/BOTTOM Boundaries: It is assumed that the values for $X$ in TOP X and $Y$ in BOTTOM Y are valid, less than or equal to the total row count, and that the specified sets of rows do not overlap. The middle segment (total rows minus $X$ minus $Y$) remains completely unsorted and maintains its original relative order.

### HASH JOIN Command
*   Distinct Column Names: As stated in the assignment document, both input tables in a JOIN are guaranteed to have mutually distinct column names. Therefore, strict table-name prefixes are not required internally to resolve ambiguities during projections or WHERE condition checks.
*   In-Memory Hash Table Abstraction: The standard std::unordered_multimap is used to build the hash table in main memory during the probing phase. It is assumed that maintaining this C++ structure strictly adheres to the rule if we restrict its underlying data population to $K - 2$ blocks at a time.
*   Grace Hash Join Fallback: While we are allowed to assume that the smaller partitioned chunks will fit into the available main memory, we have proactively added a recursive/fallback partitioning loop. If a partition still exceeds the available buffer constraints ($M = \max(1, BLOCK\_COUNT - 1)$), the system continues to partition safely rather than crashing or violating memory constraints.
*   Implicit Projection: If no PROJECT clause is provided in the query, the result table assumes a concatenation of all columns from the first table followed sequentially by all columns from the second table.

### GROUP BY Command
*   Independent Sorting for Grouping Attributes: The specification states, *"Each grouping attribute in the GROUP BY clause is processed independently"*. We interpret this to mean that the system should avoid a single-pass execution over an unsorted table (which would require fetching random blocks for each row and lead to severe I/O thrashing). Instead, the system explicitly sorts the base table into a temporary file *individually* for each requested grouping attribute before streaming it for aggregation. 
*   Handling of COUNT(*): The system internally maps COUNT(*) to track the raw row count of the group without binding it to a specific physical column index.
*   Integer Ceiling for Averages: The AVG aggregate function performs integer division but applies a ceiling operation (std::ceil((double)sum / count)) to determine the final evaluated aggregate, keeping the result compliant with the standard integer types used across the database.
*   Discarding Empty Result Tables: If the evaluation of the HAVING clause filters out all groups for a specific attribute, the resultant table is considered empty. To avoid catalog clutter and memory leaks, the system assumes it should immediately unload and delete this empty resultant table rather than registering it in the TableCatalogue.

## Contributions

- Shaunak: HASH JOIN
- Siddharth: SORT
- Shreyansh: GROUP BY, SETBUFFER K