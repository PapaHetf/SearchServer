# SearchServer
## Описание
Поиск и хранение документов, ранжирование документов по релевантности с помощью TF-IDF.  
Матчинг документов, параллельный поиск документов, поиск с учетом предиктата.
## Standart C++ 17
## Описание функционала
Метод **FindTopDocuments** в классе **SearchServer** поддерживает последовательный или  
параллельный поиск доументов по релевантности, а также поиск с учетом предиктата.  
В примере ниже показан поиск документов id которых больше 5: 
```C++
std::vector<Document> doc = search_server.FindTopDocuments(execution::par, "curly nasty cat"s, 
				[](int document_id, DocumentStatus status, int rating) { return document_id > 5; });
```
В **search_server.h** можно задать максимальное количество найденых релевантных документов:
```C++
const int MAX_RESULT_DOCUMENT_COUNT = 5;
```
Метод **MatchDocument** поддерживает последовательный и параллельный поиск и  
показывает, какие слова из запроса встречаются в документе:
```C++
const int document_id = 1;
const std::string query = "пушистый ухоженный кот -ошейник";
//...
const auto [words, status] = search_server.MatchDocument(query, document_id);
PrintMatchDocumentResult(document_id, words, status);
//...
```
Для удаления документа используйте метод **RemoveDocument**.  
Чтобы обработать параллельно несколько запросов используйте ф-цию **ProcessQueriesJoined** в файле process_queries.h:  
```C++
std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries);
```
В папке **tests** находятся тесты c времеными замерами:  
* test_find_documen.h - проверка последовательного и параллельного поиска документов;  
* test_functions.h - проверка параллельной обработки нескольких запросов и удаления докуменов;
* test_match_document.h - параллельный и последовательный матчниг документов.  
## Планы по доработке
Добиться лучшего времени выполнения при паралльных запросах.

		
