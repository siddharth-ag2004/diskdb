#pragma once

#include"syntacticParser.h"

bool semanticParse();

bool semanticParseCLEAR();
bool semanticParseCROSS();
bool semanticParseDISTINCT();
bool semanticParseEXPORT();
bool semanticParseEXPORTGraph();
bool semanticParseINDEX();
bool semanticParseJOIN();
bool semanticParseLIST();
// bool semanticParseLOAD();
bool semanticParseLOADTable();
bool semanticParseLOADGraph();

bool semanticParsePATH();
bool semanticParsePRINT();
bool semanticParsePROJECTION();
bool semanticParseRENAME();
bool semanticParseSELECTION();
bool semanticParseSORT();
bool semanticParseSOURCE();
bool semanticParseDEGREE();