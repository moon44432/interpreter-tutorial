
// MicroSEL
// interactiveMode.cpp

#include "interactiveMode.h"

void RunInteractiveShell()
{
    std::string VerStr = "v0.1.0";

    // Install standard binary operators.
    InitBinopPrec();

    // Prime the first token.
    fprintf(stderr, ("MicroSEL " + VerStr + " Interactive Shell\n\n").c_str());
    fprintf(stderr, ">>> ");
    GetNextToken(MainCode, MainIdx);

    // Run the main "interpreter loop" now.
    MainLoop(MainCode, MainIdx);
}