#pragma once

#include <vector>
using namespace std;

enum SortingStrategy
{
    ASC,
    DESC,
    NO_SORT_CLAUSE
};

struct HeapNode
{
    vector<int> row;
    int runIndex;
};

struct HeapCompare
{
    const vector<int>& columnIndices;
    const vector<SortingStrategy>& sortOrders;

    HeapCompare(const vector<int>& cols,
                const vector<SortingStrategy>& orders);

    bool operator()(const HeapNode &a, const HeapNode &b);
};

bool compareRows(
    const vector<int> &a,
    const vector<int> &b,
    const vector<int> &columnIndices,
    const vector<SortingStrategy> &sortOrders
);

void mergeRows(
    vector<vector<int>> &rows,
    int left,
    int mid,
    int right,
    const vector<int> &columnIndices,
    const vector<SortingStrategy> &sortOrders
);

void mergeSortRows(
    vector<vector<int>> &rows,
    int left,
    int right,
    const vector<int> &columnIndices,
    const vector<SortingStrategy> &sortOrders
);