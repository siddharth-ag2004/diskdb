#include "sortingStrategy.h"

HeapCompare::HeapCompare(
    const vector<int>& cols,
    const vector<SortingStrategy>& orders)
    : columnIndices(cols), sortOrders(orders) {}

bool HeapCompare::operator()(const HeapNode &a, const HeapNode &b)
{
    for (size_t i = 0; i < columnIndices.size(); i++)
    {
        int col = columnIndices[i];

        if (a.row[col] == b.row[col])
            continue;

        if (sortOrders[i] == ASC)
            return a.row[col] > b.row[col];
        else
            return a.row[col] < b.row[col];
    }

    return false;
}

bool compareRows(
    const vector<int> &a,
    const vector<int> &b,
    const vector<int> &columnIndices,
    const vector<SortingStrategy> &sortOrders)
{
    for (size_t i = 0; i < columnIndices.size(); i++)
    {
        int col = columnIndices[i];

        if (a[col] == b[col])
            continue;

        if (sortOrders[i] == ASC)
            return a[col] < b[col];
        else
            return a[col] > b[col];
    }

    return false;
}

void mergeRows(
    vector<vector<int>> &rows,
    int left,
    int mid,
    int right,
    const vector<int> &columnIndices,
    const vector<SortingStrategy> &sortOrders)
{
    int n1 = mid - left + 1;
    int n2 = right - mid;

    vector<vector<int>> L(n1);
    vector<vector<int>> R(n2);

    for (int i = 0; i < n1; i++)
        L[i] = rows[left + i];

    for (int j = 0; j < n2; j++)
        R[j] = rows[mid + 1 + j];

    int i = 0, j = 0, k = left;

    while (i < n1 && j < n2)
    {
        if (compareRows(L[i], R[j], columnIndices, sortOrders))
            rows[k++] = L[i++];
        else
            rows[k++] = R[j++];
    }

    while (i < n1)
        rows[k++] = L[i++];

    while (j < n2)
        rows[k++] = R[j++];
}

void mergeSortRows(
    vector<vector<int>> &rows,
    int left,
    int right,
    const vector<int> &columnIndices,
    const vector<SortingStrategy> &sortOrders)
{
    if (left >= right)
        return;

    int mid = left + (right - left) / 2;

    mergeSortRows(rows, left, mid, columnIndices, sortOrders);
    mergeSortRows(rows, mid + 1, right, columnIndices, sortOrders);

    mergeRows(rows, left, mid, right, columnIndices, sortOrders);
}