import os
import random
import heapq
import csv
import argparse
import sys
import shutil
import itertools

# Configuration
DEFAULT_NUM_TESTS = 5
DEFAULT_OUTPUT_DIR = "tests/cases"
MIN_NODES = 15
MAX_NODES = 50
MIN_EDGES_FACTOR = 1.5 
MAX_ATTRS = 4
MAX_WEIGHT = 20

class Graph:
    def __init__(self, name, directed=True):
        self.name = name
        self.directed = directed
        self.nodes = {} # id -> list of attrs [0, 1, ...]
        self.edges = [] # (src, dest, weight, [attrs])
        self.node_attrs_names = []
        self.edge_attrs_names = []

    def save(self, directory):
        # Save Nodes
        suffix = "D" if self.directed else "U"
        node_file = os.path.join(directory, f"{self.name}_Nodes_{suffix}.csv")
        with open(node_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(["NodeID"] + self.node_attrs_names)
            for nid, attrs in self.nodes.items():
                writer.writerow([nid] + attrs)
        
        # Save Edges
        edge_file = os.path.join(directory, f"{self.name}_Edges_{suffix}.csv")
        with open(edge_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(["Src_NodeID", "Dest_NodeID", "Weight"] + self.edge_attrs_names)
            for e in self.edges:
                writer.writerow([e[0], e[1], e[2]] + e[3])

def generate_random_graph(name, num_nodes, num_edges, num_node_attrs, num_edge_attrs):
    g = Graph(name, directed=True)
    g.node_attrs_names = [f"A{i+1}" for i in range(num_node_attrs)]
    g.edge_attrs_names = [f"B{i+1}" for i in range(num_edge_attrs)]
    
    for i in range(1, num_nodes + 1):
        g.nodes[i] = [random.choice([0, 1]) for _ in range(num_node_attrs)]
        
    possible_edges = [(u, v) for u in range(1, num_nodes + 1) for v in range(1, num_nodes + 1) if u != v]
    random.shuffle(possible_edges)
    
    for i in range(min(num_edges, len(possible_edges))):
        u, v = possible_edges[i]
        w = random.randint(1, MAX_WEIGHT)
        attrs = [random.choice([0, 1]) for _ in range(num_edge_attrs)]
        g.edges.append((u, v, w, attrs))
        
    return g

def dijkstra_no_conds(g, src):
    # Standard Dijkstra to find shortest paths without conditions
    pq = [(0, src, [src], [])]
    visited = {} # node -> cost
    paths = {} # node -> (path_nodes, path_edge_indices)

    # Build adj
    adj = {}
    for i, e in enumerate(g.edges):
        u, v, w, attrs = e
        if u not in adj: adj[u] = []
        adj[u].append((v, w, i))
    
    while pq:
        cost, u, p_nodes, p_edges = heapq.heappop(pq)
        
        if u in visited and visited[u] < cost: continue
        visited[u] = cost
        paths[u] = (p_nodes, p_edges)
        
        if u in adj:
            for v, w, idx in adj[u]:
                if v not in visited or cost + w < visited[v]:
                    heapq.heappush(pq, (cost + w, v, p_nodes + [v], p_edges + [idx]))
                    
    return paths

def solve_path(graph, src, dest, conditions):
    # ... (Keep existing solver logic) ...
    fixed_node_conds = []
    any_node_conds = []
    fixed_edge_conds = []
    any_edge_conds = []
    
    for c in conditions:
        ctype, val, attr = c
        if ctype == 'N':
            if attr: fixed_node_conds.append((attr, val))
            else: any_node_conds.append(val)
        else:
            if attr: fixed_edge_conds.append((attr, val))
            else: any_edge_conds.append(val)
            
    def check_fixed_nodes(n_attrs, names):
        for attr, val in fixed_node_conds:
            idx = names.index(attr)
            if n_attrs[idx] != val: return False
        return True

    def check_fixed_edges(e_attrs, names):
        for attr, val in fixed_edge_conds:
            idx = names.index(attr)
            if e_attrs[idx] != val: return False
        return True
    
    node_any_candidates = []
    if not any_node_conds:
        node_any_candidates = [[]]
    else:
        lists = []
        for _ in any_node_conds:
            lists.append(graph.node_attrs_names)
        node_any_candidates = list(itertools.product(*lists))

    edge_any_candidates = []
    if not any_edge_conds:
        edge_any_candidates = [[]]
    else:
        lists = []
        for _ in any_edge_conds:
            lists.append(graph.edge_attrs_names)
        edge_any_candidates = list(itertools.product(*lists))

    best_result = (float('inf'), [], [])

    for n_choice in node_any_candidates:
        for e_choice in edge_any_candidates:
            valid_nodes = set()
            for nid, attrs in graph.nodes.items():
                if not check_fixed_nodes(attrs, graph.node_attrs_names): continue
                ok = True
                for i, target_val in enumerate(any_node_conds):
                    chosen_attr = n_choice[i]
                    idx = graph.node_attrs_names.index(chosen_attr)
                    if attrs[idx] != target_val: ok = False; break
                if ok: valid_nodes.add(nid)
                
            if src not in valid_nodes or dest not in valid_nodes: continue
                
            adj = {}
            for i, e in enumerate(graph.edges):
                u, v, w, attrs = e
                if not check_fixed_edges(attrs, graph.edge_attrs_names): continue
                ok = True
                for k, target_val in enumerate(any_edge_conds):
                    chosen_attr = e_choice[k]
                    idx = graph.edge_attrs_names.index(chosen_attr)
                    if attrs[idx] != target_val: ok = False; break
                if ok:
                    if u not in adj: adj[u] = []
                    adj[u].append((v, w, i))
            
            # Dijkstra
            pq = [(0, src, [src], [])]
            visited = {}
            while pq:
                cost, u, path, edge_path = heapq.heappop(pq)
                if u in visited and visited[u] < cost: continue
                visited[u] = cost
                if u == dest:
                    if cost < best_result[0]:
                        best_result = (cost, path, edge_path)
                    break
                if u not in adj: continue
                for v, w, idx in adj[u]:
                    if v in valid_nodes:
                        if v not in visited or cost + w < visited[v]:
                            heapq.heappush(pq, (cost + w, v, path + [v], edge_path + [idx]))

    return best_result

def generate_guaranteed_condition(g, src, dest, p_nodes, p_edge_indices):
    # Analyze path to find valid conditions
    
    # 1. Fixed Node Conditions
    valid_fixed_node = [] # (attr_name, val)
    for i, attr_name in enumerate(g.node_attrs_names):
        # Check if all nodes in path have same value for this attr
        vals = [g.nodes[nid][i] for nid in p_nodes]
        if all(v == vals[0] for v in vals):
            valid_fixed_node.append((attr_name, vals[0]))
            
    # 2. Fixed Edge Conditions
    valid_fixed_edge = []
    if p_edge_indices:
        for i, attr_name in enumerate(g.edge_attrs_names):
            vals = [g.edges[eid][3][i] for eid in p_edge_indices]
            if all(v == vals[0] for v in vals):
                valid_fixed_edge.append((attr_name, vals[0]))
    else:
        # If no edges (src==dest), any edge condition is trivially true?
        # Actually logic usually requires checking all edges in path. 
        # If path has no edges, conditions on edges are vacuously true.
        # But let's avoid edge conditions if src==dest for simplicity.
        pass

    # 3. ANY Node Conditions
    # ANY(N) == 1 is valid if THERE EXISTS an attribute A_k such that for all nodes, A_k == 1.
    valid_any_node = []
    # Check 1
    can_be_1 = False
    for i in range(len(g.node_attrs_names)):
        vals = [g.nodes[nid][i] for nid in p_nodes]
        if all(v == 1 for v in vals): can_be_1 = True; break
    if can_be_1: valid_any_node.append(1)
    
    # Check 0
    can_be_0 = False
    for i in range(len(g.node_attrs_names)):
        vals = [g.nodes[nid][i] for nid in p_nodes]
        if all(v == 0 for v in vals): can_be_0 = True; break
    if can_be_0: valid_any_node.append(0)

    # 4. ANY Edge Conditions
    valid_any_edge = []
    if p_edge_indices:
        # Check 1
        can_be_1 = False
        for i in range(len(g.edge_attrs_names)):
            vals = [g.edges[eid][3][i] for eid in p_edge_indices]
            if all(v == 1 for v in vals): can_be_1 = True; break
        if can_be_1: valid_any_edge.append(1)
        
        # Check 0
        can_be_0 = False
        for i in range(len(g.edge_attrs_names)):
            vals = [g.edges[eid][3][i] for eid in p_edge_indices]
            if all(v == 0 for v in vals): can_be_0 = True; break
        if can_be_0: valid_any_edge.append(0)

    # Construct Condition List
    conds = []
    
    # Pick some random valid conditions
    # We want a mix.
    num_conds = random.randint(5, 9) 
    
    options = []
    for c in valid_fixed_node: options.append(('N', c[1], c[0]))
    for c in valid_fixed_edge: options.append(('E', c[1], c[0]))
    for v in valid_any_node: options.append(('N', v, None))
    for v in valid_any_edge: options.append(('E', v, None))
    
    if not options:
        return [] # No valid conditions found (unlikely unless attributes are chaotic)
        
    random.shuffle(options)
    return options[:num_conds]


def format_conditions(conds):
    parts = []
    for c in conds:
        ctype, val, attr = c
        name = attr if attr else "ANY"
        parts.append(f"{name}({ctype}) == {val}")
    return " AND ".join(parts) if parts else ""

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--num_tests", type=int, default=DEFAULT_NUM_TESTS)
    parser.add_argument("--output_dir", type=str, default=DEFAULT_OUTPUT_DIR)
    args = parser.parse_args()
    
    if os.path.exists(args.output_dir):
        shutil.rmtree(args.output_dir)
    os.makedirs(args.output_dir)
    
    for i in range(1, args.num_tests + 1):
        test_name = f"test_{i}"
        
        # 1. Generate Graph
        num_nodes = random.randint(MIN_NODES, MAX_NODES)
        num_edges = int(num_nodes * MIN_EDGES_FACTOR)
        g = generate_random_graph(test_name, num_nodes, num_edges, 10, 10)
        g.save(args.output_dir)
        
        # 2. Find a reachable path first
        src = random.randint(1, num_nodes)
        paths = dijkstra_no_conds(g, src)
        
        # Filter reachable nodes (excluding src if we want non-trivial paths)
        reachable = [n for n in paths.keys() if n != src]
        
        if not reachable:
            # Fallback for isolated nodes: just query src->src
            dest = src
            p_nodes, p_edges = [src], []
        else:
            dest = random.choice(reachable)
            p_nodes, p_edges = paths[dest]
            
        # 3. Generate conditions valid for this path
        conds = generate_guaranteed_condition(g, src, dest, p_nodes, p_edges)
        
        # If we couldn't find any conditions (very rare), just empty
        cond_str = format_conditions(conds)
        
        # 4. Solve (to find the *best* path satisfying these conditions, which might be different from our reference path)
        cost, path, edge_indices = solve_path(g, src, dest, conds)
        
        # 5. Generate .ra
        ra_filename = os.path.join(args.output_dir, f"{test_name}.ra")
        with open(ra_filename, 'w') as f:
            f.write(f"LOAD GRAPH {test_name} D\n")
            if cond_str:
                f.write(f"RES <- PATH {test_name} {src} {dest} WHERE {cond_str}\n")
            else:
                # Syntax for no conditions? Based on parser: "WHERE clause cannot be empty". 
                # Does parser allow omitting WHERE entirely?
                # "if (tokenizedQuery.size() > 6)" check for WHERE.
                # So omitting WHERE is fine.
                f.write(f"RES <- PATH {test_name} {src} {dest}\n")
                
            if cost != float('inf'):
                f.write("PRINT GRAPH RES\n")
            
        # 6. Generate Expected Output
        expected_filename = os.path.join(args.output_dir, f"{test_name}.expected")
        with open(expected_filename, 'w') as f:
            f.write(f"Loaded Graph. Node Count:{len(g.nodes)},Edge Count:{len(g.edges)}\n")
            if cost == float('inf'):
                f.write("FALSE\n")
            else:
                f.write(f"TRUE {cost}\n")
                
                path_nodes = path
                path_edges = [g.edges[idx] for idx in edge_indices]
                
                f.write(f"{len(path_nodes)}\n{len(path_edges)}\nD\n\n")
                
                # Nodes
                for nid in path_nodes:
                    attrs = g.nodes[nid]
                    row = [str(x) for x in [nid] + attrs]
                    f.write(",".join(row) + "\n")
                
                f.write("\n")
                
                # Edges
                for e in path_edges:
                    row = [str(x) for x in [e[0], e[1], e[2]] + e[3]]
                    f.write(",".join(row) + "\n")

if __name__ == "__main__":
    main()