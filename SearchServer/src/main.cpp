#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

#include "request_queue.h"
#include "paginator.h"
#include "process_queries.h"

#include "test_match_document.h"
#include "test_find_document.h"
#include "test_functions.h"

using namespace std;

int main() {
    SetConsoleOutputCP(1251);

    TestMatchDocument();
    TestFindTopDocument();
    TestProcessQueriesJoined();
    TestRemoveDoc();
    
    return 0;
}