#include"global.h"
/**
 * @brief File contains method to process SORT commands.
 * 
 * syntax:
 * R <- SORT relation_name BY column_name IN sorting_order
 * 
 * sorting_order = ASC | DESC 
 */
bool syntacticParseSORT() {
    logger.log("syntacticParseSORT");

    if(tokenizedQuery.size() < 6)
    {
        cout<<"SYNTAX ERROR"<<endl;
        return false;
    }

    if(tokenizedQuery[0] != "SORT")
    {
        cout<<"SYNTAX ERROR"<<endl;
        return false;
    }

    parsedQuery.queryType = SORT;
    parsedQuery.sortRelationName = tokenizedQuery[1];

    if(tokenizedQuery[2] != "BY")
    {
        cout<<"SYNTAX ERROR"<<endl;
        return false;
    }

    int i = 3;

    vector<string> columns;

    while(i < tokenizedQuery.size() && tokenizedQuery[i] != "IN")
    {
        if(tokenizedQuery[i] == ",")
        {
            i++;
            continue;
        }

        columns.push_back(tokenizedQuery[i]);
        i++;
    }

    if(columns.empty())
    {
        cout<<"SYNTAX ERROR"<<endl;
        return false;
    }

    parsedQuery.sortColumnNames = columns;

    if(i >= tokenizedQuery.size() || tokenizedQuery[i] != "IN")
    {
        cout<<"SYNTAX ERROR"<<endl;
        return false;
    }

    i++;

    vector<SortingStrategy> orders;
    parsedQuery.sortTopCount = -1;
    parsedQuery.sortBottomCount = -1;

    while(i < tokenizedQuery.size())
    {
        if(tokenizedQuery[i] == ",")
        {
            i++;
            continue;
        }

        if(tokenizedQuery[i] == "ASC")
        {
            orders.push_back(ASC);
            i++;
        }
        else if(tokenizedQuery[i] == "DESC")
        {
            orders.push_back(DESC);
            i++;
        }
        else if(tokenizedQuery[i] == "TOP")
        {
            i++;
            if(i >= tokenizedQuery.size())
            {
                cout<<"SYNTAX ERROR"<<endl;
                return false;
            }

            try
            {
                parsedQuery.sortTopCount = stoi(tokenizedQuery[i]);
            }
            catch(...)
            {
                cout<<"SYNTAX ERROR"<<endl;
                return false;
            }
            i++;
        }
        else if(tokenizedQuery[i] == "BOTTOM")
        {
            i++;
            if(i >= tokenizedQuery.size())
            {
                cout<<"SYNTAX ERROR"<<endl;
                return false;
            }
            try
            {
                parsedQuery.sortBottomCount = stoi(tokenizedQuery[i]);
            }
            catch(...)
            {
                cout<<"SYNTAX ERROR"<<endl;
                return false;
            }
            i++;
        }
        else
        {
            cout<<"SYNTAX ERROR"<<endl;
            return false;
        }
    }

    if(columns.size() != orders.size())
    {
        cout<<"SYNTAX ERROR"<<endl;
        return false;
    }

    parsedQuery.sortStrategies = orders;

    return true;
}

bool semanticParseSORT() {
    logger.log("semanticParseSORT");

    if(!tableCatalogue.isTable(parsedQuery.sortRelationName))
    {
        cout<<"SEMANTIC ERROR: Relation doesn't exist"<<endl;
        return false;
    }

    for(string column : parsedQuery.sortColumnNames)
    {
        if(!tableCatalogue.isColumnFromTable(column, parsedQuery.sortRelationName))
        {
            cout<<"SEMANTIC ERROR: Column doesn't exist in relation"<<endl;
            return false;
        }
    }

    return true;
}

void executeSORT() {
    logger.log("executeSORT");

    Table *table = tableCatalogue.getTable(parsedQuery.sortRelationName);

    vector<int> columnIndices;

    for(string column : parsedQuery.sortColumnNames)
    {
        columnIndices.push_back(table->getColumnIndex(column));
    }

    table->externalSortCreateNewTable(
        parsedQuery.sortRelationName,
        columnIndices,
        parsedQuery.sortStrategies,
        parsedQuery.sortTopCount,
        parsedQuery.sortBottomCount
    );
}