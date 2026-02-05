#include "global.h"
/**
 * @brief File contains method to process PATH commands.
 * 
 * syntax:
 * RES <- PATH <graph_name> <src_NodeID> <dest_NodeID> WHERE <conditions>
 * 
 */
bool syntacticParsePATH()
{
    logger.log("syntacticParsePATH");

    if (tokenizedQuery.size() < 6 || tokenizedQuery[1] != "<-" || tokenizedQuery[2] != "PATH")
    {
        cout << "SYNTAX ERROR: INVALID PATH SYNTAX" << endl;
        return false;
    }

    parsedQuery.queryType = PATH;
    parsedQuery.pathResultGraphName = tokenizedQuery[0];
    parsedQuery.pathGraphName = tokenizedQuery[3];

    try {
        parsedQuery.pathSrcNodeId = stoi(tokenizedQuery[4]);
        parsedQuery.pathDestNodeId = stoi(tokenizedQuery[5]);
    } catch (...) {
        cout << "SYNTAX ERROR: Source and Destination IDs must be integers" << endl;
        return false;
    }

    if (tokenizedQuery.size() > 6)
    {
        if (tokenizedQuery[6] != "WHERE")
        {
            cout << "SYNTAX ERROR: Expected WHERE clause" << endl;
            return false;
        }

        if (tokenizedQuery.size() == 7)
        {
            cout << "SYNTAX ERROR: WHERE clause cannot be empty" << endl;
            return false;
        }

        int i = 7;
        while (i < tokenizedQuery.size())
        {
            string token = tokenizedQuery[i];

            if (token == "AND")
            {
                cout << "SYNTAX ERROR: Unexpected keyword 'AND'" << endl;
                return false;
            }

            ParsedQuery::PathCondition condition;

            size_t openParen = token.find('(');
            size_t closeParen = token.find(')');

            if (openParen == string::npos || closeParen == string::npos || 
                closeParen < openParen || closeParen != token.length() - 1)
            {
                cout << "SYNTAX ERROR: Invalid condition format '" << token << "'. Ensure spaces between condition and operator." << endl;
                return false;
            }



            condition.attribute = token.substr(0, openParen);
            string typeStr = token.substr(openParen + 1, closeParen - openParen - 1);

            if (typeStr != "N" && typeStr != "E")
            {
                cout << "SYNTAX ERROR: Condition type must be N or E" << endl;
                return false;
            }
            condition.type = typeStr[0];
            
            i++; 

            if (i < tokenizedQuery.size() && tokenizedQuery[i] == "==")
            {
                i++; 
                
                if (i >= tokenizedQuery.size())
                {
                    cout << "SYNTAX ERROR: Expected value after '=='" << endl;
                    return false;
                }
                
                string valStr = tokenizedQuery[i];
                if (valStr != "0" && valStr != "1")
                {
                    cout << "SYNTAX ERROR: Condition value must be 0 or 1" << endl;
                    return false;
                }

                condition.value = stoi(valStr);
                condition.isExplicit = true;
                i++; 
            }
            else
            {
                condition.value = -1; 
                condition.isExplicit = false;
            }

            

            parsedQuery.pathConditions.push_back(condition);

            if (i < tokenizedQuery.size())
            {
                if (tokenizedQuery[i] == "AND")
                {
                    i++; 
                    
                    if (i >= tokenizedQuery.size())
                    {
                        cout << "SYNTAX ERROR: Query cannot end with 'AND'" << endl;
                        return false;
                    }
                }
                else
                {
                    cout << "SYNTAX ERROR: Expected 'AND' between conditions" << endl;
                    return false;
                }
            }
        }
    }



    return true;
}

bool semanticParsePATH()
{
    logger.log("semanticParsePATH");

 
    if (graphCatalogue.isGraph(parsedQuery.pathResultGraphName, DIRECTED) || 
        graphCatalogue.isGraph(parsedQuery.pathResultGraphName, UNDIRECTED))
    {
        cout << "SEMANTIC ERROR: Resultant graph already exists" << endl;
        return false;
    }

  
    Graph* graph = nullptr;
    if (graphCatalogue.isGraph(parsedQuery.pathGraphName, DIRECTED))
    {
        graph = graphCatalogue.getGraph(parsedQuery.pathGraphName, DIRECTED);
    }
    else if (graphCatalogue.isGraph(parsedQuery.pathGraphName, UNDIRECTED))
    {
        graph = graphCatalogue.getGraph(parsedQuery.pathGraphName, UNDIRECTED);
    }

    if (graph == nullptr)
    {
        cout << "SEMANTIC ERROR: Graph doesn't exist" << endl;
        return false;
    }

    for (const auto& cond : parsedQuery.pathConditions)
    {
        if (cond.attribute == "ANY") continue;

        if (cond.type == 'N')
        {
            if (!graph->nodeTable->isColumn(cond.attribute))
            {
                cout << "SEMANTIC ERROR: Node attribute " << cond.attribute << " does not exist" << endl;
                return false;
            }
        }
        else if (cond.type == 'E')
        {
            if (!graph->edgeTable->isColumn(cond.attribute))
            {
                cout << "SEMANTIC ERROR: Edge attribute " << cond.attribute << " does not exist" << endl;
                return false;
            }
        }
    }


    return true;
}

