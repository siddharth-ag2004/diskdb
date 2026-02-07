# Project Phase 1 Report: Graph Extension for SimpleRA

## 1. Overview
Here we cover the implementation of LOAD, EXPORT, PRINT, DEGREE, and PATH operations for both Directed and Undirected graphs. We have strictly adhered to the 2-block memory access limit enforced by the buffer manager.

## 2. Implementation Details

We utilized the provided Table, Page, BufferManager, and Cursor classes for low-level data handling. Our work focused on the Graph class, the ExternalSort logic, and the Executors for specific commands.

### 2.1 LOAD GRAPH
Logic:
1.  Raw Loading: The system first loads the Node and Edge CSV files into temporary Table objects using the existing Table::load() method. At this stage, data is in a Heap (unsorted) format.
2.  External Sorting (The "Three-Table" Strategy):
    To support efficient queries, the system immediately reorganizes the raw data into three distinct, physically sorted tables:
    *   _Nodes_Sorted: Sorted by NodeID. Used for checking node existence and retrieving node attributes.
    *   _Edges_Sorted_Src (Forward Table): Sorted by Src_NodeID. This acts as a Clustered Index on the source, grouping all outgoing edges of a node into contiguous blocks.
    *   _Edges_Sorted_Dest (Reverse Table): Sorted by Dest_NodeID. This acts as a Clustered Index on the destination, crucial for calculating In-Degree and traversing undirected graphs efficiently.

External Sort Algorithm:
We implemented a Two-Phase Multi-Way Merge Sort (Table::externalSortCreateNewTable):
*   Phase 1 (Run Generation): The table is read page-by-page. Each page is sorted in memory and written back as a temporary "run" file.
*   Phase 2 (K-Way Merge): A Min-Heap (priority_queue) is used to merge these sorted runs into a final sorted table. This ensures we strictly respect the memory limit, as we only need 1 block per run in memory (handled by the BufferManager's replacement policy).

Complexity:
*   Time: $O(N \log N)$ where $N$ is the number of rows.
*   Block Accesses: $2 \cdot B \cdot (1 + \lceil \log_{M-1}(B/M) \rceil)$, where $B$ is total blocks and $M$ is buffer size. While heavy on writes initially, this reduces the complexity of PATH queries from linear $O(B)$ to logarithmic/constant time.

### 2.2 DEGREE
Logic:
For a given node $u$:
1.  Out-Degree: We perform a Binary Search on _Edges_Sorted_Src to find the first occurrence of Src_NodeID == u. We then scan sequentially until the Source ID changes.
2.  In-Degree: We perform a Binary Search on _Edges_Sorted_Dest to find the first occurrence of Dest_NodeID == u. We scan sequentially until the Dest ID changes.
3.  Result: Degree = Out-Degree + In-Degree (for Directed) or the sum of edges found in both (for Undirected, noting the bidirectional assumption).

Block Accesses:
*   Search: $\log_2(B_{Edges})$ to find the start.
*   Scan: $\lceil \text{Degree}(u) / \text{RowsPerBlock} \rceil$.
*   Total: $\approx 2 \times \log_2(B)$ blocks read. This is significantly more efficient than a linear scan of the whole table ($B$ blocks).

### 2.3 PATH (Dijkstra's Algorithm)
Logic:
We implemented Dijkstra’s algorithm using a priority queue.
1.  Initialization: Check if Source/Dest nodes exist using Binary Search on _Nodes_Sorted.
2.  Traversal: To expand a node $u$:
    *   We call cursorToNeighbors(u). This function performs a binary search on _Edges_Sorted_Src to return a cursor positioned exactly at the first edge starting from $u$.
    *   If the graph is Undirected, we also search _Edges_Sorted_Dest to find edges where $u$ is the destination (treated as outgoing).
3.  Condition Checking (WHERE clause):
    *   Attributes: When an edge or node is visited, we retrieve its row. PathCondition logic checks if attributes match the query.
    *   ANY Handling:
        *   ANY(N): The attribute value of the Source Node is read first. The condition is converted to an explicit equality check (e.g., A1 == 0) for all subsequent nodes.
        *   ANY(E): Since the first edge is not known upfront, the system generates execution plans. It runs Dijkstra assuming ANY=0 and again assuming ANY=1, returning the best valid path found.

Block Access & Efficiency:
*   Neighbor Retrieval: Because edges are sorted by Source, all neighbors of $u$ reside in contiguous blocks.
    *   Cost: $\approx 1$ Random Access (Binary Search) + 1 Sequential Read (to fetch neighbors).
*   Node Attribute Access: Checking a node condition requires fetching node details.
    *   Cost: 1 Random Access (Binary Search on _Nodes_Sorted).

### 2.4 EXPORT and PRINT
These commands wrap the underlying Table operations.
*   EXPORT: Calls makePermanent(), renaming the temporary page files to the required CSV format in /data.
*   PRINT: Iterates through the tables using a Cursor and prints rows.

---

## 3. Page Design and Storage Justification

### 3.1 Page Design
We utilized the provided Page class which implements a Fixed-Length Record design.
*   Structure: Header (RowCount) + Data (Vector of Integers).
*   Why Appropriate: Graph attributes and IDs are integers. Fixed-length rows allow O(1) calculation of offsets ($BlockID \times RowsPerBlock + Offset$). This is essential for the Binary Search logic used in findFirstOccurrence.

### 3.2 Storage Strategy: Clustered Index vs. SINK (Secondary Index on Non-Key)

The most critical design choice was storing the graph as three physically sorted tables (Clustered Indexes) rather than keeping a single Heap file and building Secondary Indexes.

Comparison Scenario:
*   Graph: $10^5$ Edge Blocks.
*   Query: Find neighbors of Node $U$.
*   Stats: Node $U$ has 50 outgoing edges. A block holds 1000 edges.

#### Option A: Our Approach (Clustered Index / Sorted File)
The table is physically sorted by Src_NodeID.
1.  Locality: All 50 edges for Node $U$ are stored sequentially.
2.  Access:
    *   Binary Search to find the block containing the first edge ($\log_2 10^5 \approx 17$ accesses, usually cached).
    *   Read the block. All 50 edges are in this specific block.
3.  Total I/O: 1 Data Block Read.

#### Option B: SINK (Secondary Index on Non-Key without B-Tree)
A SINK structure consists of:
1.  Dense Index File: Sorted pairs of <Key, Pointer_to_Indirection>.
2.  Indirection Blocks: Buckets containing lists of RowIDs (pointers to the Heap).
3.  Heap File: The actual edge data, unsorted.

Access Steps:
1.  Index Search: Binary Search the Index File to find $U$. ($\approx \log_2 (\text{IndexBlocks})$).
2.  Indirection: Read the Indirection block to get the 50 RowIDs. (1 Block).
3.  Data Retrieval (The Bottleneck):
    *   The 50 RowIDs point to the Heap File.
    *   Because the Heap File is unsorted (edges were inserted in arbitrary order), Edge 1 might be in Block 10, Edge 2 in Block 500, Edge 3 in Block 5000.
    *   In the worst case, this requires 50 distinct Block Reads.
    *   With a Buffer Manager size of 2, this causes massive thrashing.

#### Justification Conclusion
For graph traversals (Dijkstra/BFS), spatial locality is paramount.
*   Clustered Index Cost: $O(1)$ block access per node expansion.
*   SINK Cost: $O(Degree)$ block accesses per node expansion.

Given the constraint of 2 blocks in memory, the SINK approach would result in extreme performance degradation due to disk thrashing. Therefore, the overhead of sorting during LOAD is mathematically justified by the exponential speedup in PATH and DEGREE operations.

---

## 4. Error Handling

1.  File Existence: Checked via stat() before attempting LOAD.
2.  Entity Resolution: The system distinguishes between Tables and Graphs in the catalogue. Attempting LOAD GRAPH on an existing name triggers a semantic error.
3.  Parsing:
    *   Validates U/D flags.
    *   Ensures numeric types for IDs.
    *   Parses complex WHERE clauses, checking for balanced parentheses and valid operators.
4.  Runtime Logic:
    *   DEGREE returns error if the node does not exist.
    *   PATH verifies source/destination existence before starting Dijkstra.
    *   Semantic checks ensure attributes in WHERE clauses actually exist in the schema.

---

## 5. Assumptions

1.  Identifier Type: All NodeIDs and attributes are integers.
2.  Undirected Graphs: The input CSV for an undirected graph contains edges only once (e.g., 1,2). The system assumes implied bi-directionality, meaning a query for neighbors of 2 must find the row 1,2.
3.  Intermediate Memory: As per project guidelines ("you may store intermediate data structures in memory"), we assume the priority_queue used in Dijkstra and the map used for distances fit in main memory, provided the actual data blocks are accessed via the Buffer Manager.
4.  Buffer Manager Policy: We assume the provided Buffer Manager correctly evicts the oldest page (FIFO) when a 3rd page is requested, effectively enforcing the 2-block limit.
5.  Attribute Consistency: For ANY(N) conditions, we assume the attribute value of the Source Node dictates the required value for the entire path. For ANY(E), we assume the path is valid if it satisfies consistency with either 0 or 1.