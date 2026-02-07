# Project Phase 1 Report: Graph Extension for SimpleRA

## 1. Overview

Here we cover the implementation of LOAD, EXPORT, PRINT, DEGREE, and PATH operations for both Directed and Undirected graphs. We have strictly adhered to the 2-block memory access limit enforced by the buffer manager.

## 2. Implementation Details

We utilized the provided Table, Page, BufferManager, and Cursor classes for low-level data handling. Most of our implementation is in the Graph class, the ExternalSort logic, and the Executors for specific commands.

### 2.1 LOAD GRAPH

Logic:

1.  Raw Loading: The system first loads the Node and Edge CSV files into temporary Table objects using the existing Table::load() method. At this stage, data is in a Heap (unsorted) format.
2.  External Sorting (The "Three-Table" Strategy):
    To support efficient queries, the system immediately reorganizes the raw data into three distinct, physically sorted tables:
    - \_Nodes_Sorted: Sorted by NodeID. Used for checking node existence and retrieving node attributes.
    - \_Edges_Sorted_Src (Forward Table): Sorted by Src_NodeID. This acts as a Clustered Index on the source, grouping all outgoing edges of a node into contiguous blocks.
    - \_Edges_Sorted_Dest (Reverse Table): Sorted by Dest_NodeID. This acts as a Clustered Index on the destination, crucial for calculating In-Degree and traversing undirected graphs efficiently.

External Sort Algorithm:
We implemented a Two-Phase Multi-Way Merge Sort (Table::externalSortCreateNewTable):

- Phase 1 (Run Generation): The table is read page-by-page. Each page is sorted in memory and written back as a temporary "run" file.
- Phase 2 (K-Way Merge): A Min-Heap (priority_queue) is used to merge these sorted runs into a final sorted table. This ensures we strictly respect the memory limit, as we only need 1 block per run in memory (handled by the BufferManager's replacement policy).

Complexity:

- Time: $O(N \log N)$ where $N$ is the number of rows.
- Block Accesses: $2 \cdot B \cdot (1 + \lceil \log_{M-1}(B/M) \rceil)$, where $B$ is total blocks and $M$ is buffer size. While heavy on writes initially, this reduces the complexity of PATH queries from linear $O(B)$ to logarithmic/constant time.

### 2.2 EXPORT

The EXPORT command is responsible for persisting temporary results (tables or graphs created during query execution) or loaded entities into the permanent /data/ directory.

Implementation Logic:

1.  Entity Identification: The executor checks the ParsedQuery to determine if the target is a TABLE or a GRAPH.
2.  Delegation to Table/Graph Objects:
    - Table Export: Table::makePermanent() is called. This function:
      - Constructs the destination file path (../data/<Name>.csv).
      - Opens an output file stream (ofstream).
      - Writes the column headers first.
      - Initializes a Cursor at page 0 of the table.
      - Iterates through all rows using cursor.getNext(). This method transparently handles page boundaries, loading one block at a time into the buffer and evicting the previous one.
      - Writes each row to the CSV stream using writeRow.
      - Finally, deletes the temporary source file (if one existed in /data/temp/) via BufferManager::deleteFile.
    - Graph Export: Graph::makePermanent() delegates the operation to its constituent parts. It calls makePermanent() on the internal nodeTable and edgeTable.
      - Note: The system exports the original structure tables (nodeTable and edgeTable), not the sorted clustered index tables used for optimization. This ensures the exported CSVs match the input format required by the specification.

Block Access Analysis:
The operation requires a full sequential scan of the entity.

- Reads: $N$ blocks (where $N$ is the total number of blocks in the table/graph).
- Writes: 0 Block Writes (data is written directly to the OS file stream, not the Buffer Manager).

### 2.3 PATH (Dijkstra's Algorithm)

The PATH command determines the shortest path between two nodes subject to specific attribute constraints. We implemented Dijkstra’s Algorithm optimized for disk-based storage using Clustered Indexes and our Two-Tier Binary Search.

Implementation Logic:

1.  Condition Parsing & Execution Plans:
    - Attributes: The WHERE clause is parsed into PathCondition structs.
    - ANY(N) Handling: Before starting the search, the system retrieves the Source Node's attributes. The ANY condition is immediately resolved to a concrete equality check (e.g., Attribute_1 == 0) to ensure consistency across the path.
    - ANY(E) Handling: Since the first edge is unknown, the system generates branching execution plans. It runs Dijkstra assuming ANY=0 and, if necessary, again assuming ANY=1, returning the optimal valid result.

2.  Dijkstra's Traversal:
    - Initialization: We verify the existence of Source and Destination nodes using cursorToNode.
    - Expansion: A Min-Heap (priority_queue) manages the frontier. To expand node $u$:
      - Neighbor Retrieval: We call cursorToNeighbors(u). This uses the Forward Edge Table (\_Edges_Sorted_Src).
      - Undirected Graphs: The system effectively performs a "union" of neighbors by also searching the Reverse Edge Table (\_Edges_Sorted_Dest) where $u$ appears as the destination.
    - Filtering:
      - Edge Conditions: Checked immediately against the current row in the edge table cursor.
      - Node Conditions: Before adding a neighbor $v$ to the queue, we verify its attributes by performing a lookup (cursorToNode(v)) in the Node Table.

#### Block Access & Efficiency Analysis:

The efficiency of PATH relies entirely on minimizing the I/O required to fetch neighbors and check node attributes.

- Neighbor Fetch Cost:
  To find the adjacency list of $u$, we perform findFirstOccurrence on the sorted edge table.
  - Using the Optimized Two-Tier Binary Search, we search the _Block Space_ rather than the Row Space.
  - Cost: $\lceil \log_2(B_E) \rceil + \lceil \text{Degree}(u) / \text{RowsPerBlock} \rceil$ block accesses.
  - Note: Since edges are physically clustered by Source Node, $\lceil \text{Degree}(u) / \text{RowsPerBlock} \rceil$ is typically just 1 for average degrees.

- Node Attribute Check Cost:
  To check conditions on a neighbor $v$, we look it up in \_Nodes_Sorted.
  - Cost: $\lceil \log_2(B_N) \rceil$ block accesses.

- Comparison:
  Without the Two-Tier Binary Search optimization, a standard binary search over global row IDs would incur $\approx \log_2(N)$ random I/Os (jumping between pages).
  - With $N = 10^8$ rows and $B = 10^5$ blocks:
    - Standard Search: $\approx 27$ I/Os per lookup.
    - Two-Tier Search: $\approx 17$ I/Os per lookup.
  - This reduction of ~10 I/Os per node expansion massively improves the runtime for large graph traversals.

Here is the updated section 2.4 DEGREE with the refined block access analysis based on the Two-Tier Binary Search.

### 2.4 DEGREE

The DEGREE command computes the total number of edges incident to a specific node. For directed graphs, this is the sum of in-degree and out-degree. For undirected graphs, it implies checking for connections in both directions (due to the bidirectional nature of edges).

Implementation Logic:

1.  Out-Degree Calculation:
    - We access the Forward Edge Table (\_Edges_Sorted_Src), which acts as a Clustered Index on Src_NodeID.
    - We use findFirstOccurrence to locate the first row where Src_NodeID == u.
    - We use findFirstOccurrence to locate the first row where Src_NodeID == u + 1.
    - The difference in row indices gives the count. (Or implicitly, we scan from the start until the ID changes).
2.  In-Degree Calculation:
    - We access the Reverse Edge Table (\_Edges_Sorted_Dest), which acts as a Clustered Index on Dest_NodeID.
    - We apply the same logic: locate the range of rows where Dest_NodeID == u.
3.  Result: The sum of counts from both steps is returned.

Block Access & Efficiency Analysis:
The efficiency is derived from the Two-Tier Binary Search minimizing random accesses to finding the start of the range, and the Clustered Storage ensuring the scan is sequential.

Let $B_E$ be the total number of edge blocks and $R$ be the rows per block.

- Search Cost (Finding the boundary):
  Using the Two-Tier Binary Search, we search the _Block Space_ rather than the Row Space.
  - Cost per table: $\lceil \log_2(B_E) \rceil$ block accesses.
  - This locates the specific page containing the start of Node $u$'s edges.
- Scan Cost (Counting the edges):
  Since the table is sorted by the search key, all edges for Node $u$ are stored contiguously.
  - Cost per table: $\lceil \text{Degree}(u) / R \rceil$ block accesses.
  - _Note:_ For a typical graph where $\text{Degree}(u) \ll R$ (e.g., degree 50 vs 1000 rows/block), this is effectively 1 block read.
- Total Complexity:
  $$ \text{Total I/O} \approx 2 \times \left( \lceil \log_2(B_E) \rceil + 1 \right) $$
    For a graph with $10^8$ edges ($10^5$ blocks):
  - Linear Scan: $100,000$ I/Os
  - Standard Binary Search: $\approx 2 \times 27 = 54$ I/Os.
  - Our Two-Tier Approach: $\approx 2 \times 17 = 34$ I/Os.

### 2.5 PRINT

The PRINT command outputs the contents of a relation or graph to standard output (cout).

Implementation Logic of Graph::print:

- Metadata: The system first prints the header information: NodeCount, EdgeCount, and Type (D/U), separated by newlines.
- Full Content Dump: Unlike tables, graphs are printed in their entirety as per the specification.
  - Nodes: The system iterates through nodeTable using a Cursor from $i = 0$ to nodeCount. Every row is printed.
  - Separator: A blank line is printed.
  - Edges: The system iterates through edgeTable using a separate Cursor from $i = 0$ to edgeCount. Every row is printed.
- Memory Efficiency: Despite potentially printing millions of lines, the Cursor abstraction ensures that only one page of the graph is in memory at any given instant. The BufferManager swaps pages automatically as the printing loop crosses block boundaries.

Block Access Analysis: $B_N + B_E$ Block Reads (Sequential scan of all Node blocks and Edge blocks).

### 2.6 Binary Search Implementation

To support efficient DEGREE computations and PATH existence checks, we rely heavily on searching for specific keys (NodeIDs) within our sorted tables. A naive binary search over the global row index range $[0, TotalRows]$ would result in random page accesses jumping across the file, causing buffer thrashing.

Instead, we implemented a Two-Tier Binary Search: first searching the Block Space, then searching the Row Space.

Since the tables (_Nodes_Sorted, \_Edges_Sorted_Src, etc.) are physically sorted, the last row of any page $P_i$ acts as a separator for $P_{i+1}$.

1.  Phase 1 (Page Search): We perform a binary search on the page indices $[0, BlockCount-1]$. For a middle page midPage, we fetch the last row.
    - If LastRow[Col] >= Target, the target value might be in this page (or an earlier one). We record this page as a candidate and move High = mid - 1.
    - If LastRow[Col] < Target, the target must be in a later page. We move Low = mid + 1.
2.  Phase 2 (Row Search): Once the specific TargetPage is identified, we load it into memory and perform a standard in-memory binary search on the rows within that single page to find the exact index.

#### Minimizing Block Accesses:

Let $N$ be the total number of rows and $R$ be the rows per block. The total blocks $B = N/R$.

- Global Binary Search: Searching purely by row index requires $\lceil \log_2 N \rceil$ accesses. In the worst case, every jump lands on a different page, resulting in $\approx \log_2 N$ disk I/Os.
- Two-Tier Search:
  - We search the pages: $\lceil \log_2 B \rceil$ disk I/Os.
  - We search the rows: $0$ additional disk I/Os (the target page is already in the buffer).
  - Total I/O: $\log_2 (N/R) = \log_2 N - \log_2 R$.

Example:
For $10^8$ edges and 1000 edges/block:

- Global Search: $\log_2(10^8) \approx 27$ I/Os.
- Page Search: $\log_2(10^5) \approx 17$ I/Os.
- Result: A savings of ~10 I/Os per search. For a complex query traversing thousands of nodes, this optimization significantly reduces total disk latency.

### 2.7 External Merge Sort Implementation

To support the creation of Primary Index for node and Clustered Index for edges (the sorted tables required for the graph), we implemented a Two-Phase External Merge Sort. This algorithm is designed to sort datasets larger than the available buffer memory by breaking the process into sorting individual blocks followed by a merge step.

The implementation in Table::externalSortCreateNewTable strictly adheres to the constraint of having a maximum of 2 blocks loaded in the BufferManager at any time.

#### Phase 1: Sorted Run Generation

In this phase, we iterate through the source table one block at a time to create initial sorted "runs".

Logic:

1.  Iterative Loading: The algorithm loops through the source table pages (pageIdx = 0 to blockCount).
2.  In-Memory Sort:
    - A single page is requested via bufferManager.getPage().
    - The rows are extracted into a standard C++ vector (rows).
    - We use std::sort with a custom lambda comparator to sort these rows based on the target columnIndex (e.g., Src_NodeID).
3.  Run Writing:
    - The sorted rows are written immediately to a new temporary table (a "run") named \_run_X.
    - Each run consists of exactly one sorted block.

Memory Compliance:
During this phase, exactly one source page is active in the buffer pool. The write operation utilizes the second available slot in the buffer pool (if needed) or flushes the source page. This ensures we never exceed the 2-block limit.

#### Phase 2: K-Way Merge

In this phase, we merge all the 1-block sorted runs created in Phase 1 into a single final sorted table.

Logic:

1.  Cursor Initialization: We instantiate a Cursor for every run table generated in Phase 1.
2.  Min-Heap (Priority Queue): We utilize a Min-Heap (priority_queue) to store the current smallest row from each of the $K$ runs. The heap stores {SortKey, RowData, RunIndex}.
3.  Merge Process:
    - Initially, the first row from every cursor is pushed into the heap.
    - In a loop, we extract the minimum element (the "winner") from the heap and add it to an output buffer (outputRows) in main memory.
    - We advance the cursor of the "winner" run (runCursors[node.runIndex].getNext()) to fetch the next row from that specific run and push it into the heap.
4.  Output Flushing: When outputRows fills a block (reaches maxRowsPerBlock), we request the BufferManager to write this page to the final result table.

Memory Compliance:
Although we are merging $K$ runs (where $K$ could be $>2$), the system does not load $K$ pages simultaneously.

- On-Demand Loading: The Cursor object is lightweight and does not hold the physical page. It only requests a page from the BufferManager when getNext() is called.
- Buffer Swapping: Since we interpret the rows sequentially, the BufferManager (using FIFO policy) evicts the least recently used block to load the block required by the current cursor.
- Result Writing: The output is accumulated in a memory vector (allowed by assumptions) and written to disk as a single block write only when full.

Complexity Analysis:

- I/O Cost:
  - Phase 1: Read $N$ blocks, Write $N$ blocks.
  - Phase 2: Read $N$ blocks (via cursors), Write $N$ blocks (final result).
  - Total: $4N$ Block I/Os.
  - This linear I/O complexity $O(N)$ is optimal for external sorting given the strict memory constraints.

## 3. Page Design and Storage Justification

### 3.1 Page Design

We utilized the provided Page class which implements a Fixed-Length Record design.

- Structure: Header (RowCount) + Data (Vector of Integers).
- Why Appropriate: Graph attributes and IDs are integers. Fixed-length rows ensure that every block has a predictable structure. Most importantly, this design allows the Two-Tier Binary Search to instantly retrieve the last row of any page (the boundary value) using a calculated offset, without needing to deserialize or scan the entire page. This capability is the foundation of the $O(\log B)$ block search optimization.

### 3.2 Storage Strategy: Clustered Index vs. SINK (Secondary Index on Non-Key)

The most critical design choice was storing the graph as three physically sorted tables (Clustered Indexes) rather than keeping a single Heap file and building Secondary Indexes.

Comparison Scenario:

- Graph: $10^5$ Edge Blocks.
- Query: Find neighbors of Node $U$.
- Stats: Node $U$ has 50 outgoing edges. A block holds 1000 edges.

#### Option A: Our Approach (Clustered Index / Sorted File)

The table is physically sorted by Src_NodeID.

1.  Locality: All 50 neighbors for Node $U$ are stored physically adjacent to each other (likely in the same block).
2.  Access:
    *   Search Phase: We use the Two-Tier Binary Search (Page-Level) to find the specific block containing the start of Node $U$'s edges.
        *   Cost: $\lceil \log_2(10^5) \rceil \approx 17$ Block Reads.
    *   Retrieval Phase: Once the target block is identified, we read it. Since the data is clustered, all 50 edges are present in this single block (or at most spanning to the immediate next block).
        *   Cost: 1 Block Read.
3.  Total Cost: $\approx 18$ Block Accesses (17 Search + 1 Retrieval).

#### Option B: SINK (Secondary Index on Non-Key without B-Tree)

A SINK structure consists of:
1.  Dense Index File: Sorted pairs of <Key, Pointer_to_Indirection>.
2.  Indirection Blocks: Buckets containing lists of RowIDs (pointers to the Heap).
3.  Heap File: The actual edge data, unsorted.

Access Steps:
1.  Index Search: Binary Search the Index File to find $U$.
    *   Cost: $\approx \log_2 (\text{IndexBlocks}) \approx 17$ Block Reads.
2.  Indirection: Read the Indirection block to get the 50 RowIDs.
    *   Cost: 1 Block Read.
3.  Data Retrieval (The Bottleneck):
    *   The 50 RowIDs point to specific locations in the Heap File.
    *   Because the Heap File is unsorted (edges were inserted in arbitrary order), Edge 1 might be in Block 10, Edge 2 in Block 500, etc.
    *   Cost: In the worst case, this requires 50 distinct Block Reads.
    *   With a Buffer Manager size of 2, this causes massive thrashing as no blocks are retained.

#### Justification Conclusion

For graph traversals (Dijkstra/BFS), spatial locality is the deciding factor.

*   Clustered Index Retrieval: $O(1)$ sequential block access per node expansion (after search).
*   SINK Retrieval: $O(Degree)$ random block accesses per node expansion (after search).

While both methods incur similar search costs ($\approx 17$ I/Os), the SINK approach fails during the retrieval phase, potentially requiring $50\times$ more I/O operations for a node with degree 50. Given the strict constraint of 2 blocks in memory, the SINK approach would result in extreme performance degradation. Therefore, the overhead of sorting during LOAD is mathematically justified.

## 4. Error Handling

1.  File Existence: Checked via stat() before attempting LOAD.
2.  Entity Resolution: The system distinguishes between Tables and Graphs in the catalogue. Attempting LOAD GRAPH on an existing name triggers a semantic error.
3.  Parsing:
    - Validates U/D flags.
    - Ensures numeric types for IDs.
    - Parses complex WHERE clauses, checking for balanced parentheses and valid operators.
4.  Runtime Logic:
    - DEGREE returns error if the node does not exist.
    - PATH verifies source/destination existence before starting Dijkstra.
    - Semantic checks ensure attributes in WHERE clauses actually exist in the schema.

## 5. Assumptions

1.  Identifier Type: All NodeIDs and attributes are integers.
2.  Undirected Graphs: The input CSV for an undirected graph contains edges only once (e.g., 1,2). The system assumes implied bi-directionality, meaning a query for neighbors of 2 must find the row 1,2.
3.  Intermediate Memory: As per project guidelines ("you may store intermediate data structures in memory"), we assume the priority_queue used in Dijkstra and the map used for distances fit in main memory, provided the actual data blocks are accessed via the Buffer Manager.
4.  Buffer Manager Policy: We assume the provided Buffer Manager correctly evicts the oldest page (FIFO) when a 3rd page is requested, effectively enforcing the 2-block limit.
5.  Attribute Consistency: For ANY(N) conditions, we assume the attribute value of the Source Node dictates the required value for the entire path. For ANY(E), we assume the path is valid if it satisfies consistency with either 0 or 1.
