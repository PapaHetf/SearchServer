#pragma once
#include "search_server.h"
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate);

    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);

    vector<Document> AddFindRequest(const string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        QueryResult() = default;
        int time = 0;
        int count = 0; //только пустые запросы
    };
    deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
    const SearchServer& search_server_;
    QueryResult Qstr;
};

template <typename DocumentPredicate>
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
    vector<Document> result_search = search_server_.FindTopDocuments(raw_query);
    if (Qstr.time != sec_in_day_) {
        ++Qstr.time;
        if (result_search.empty()) {
            ++Qstr.count;   // кол-во пустых запросов
        }
        requests_.push_back(Qstr);
        return result_search;
    }
    
    requests_.pop_front();  //удаляем устаревший запрос
    if (result_search.empty()) {
        if (requests_.back().count < sec_in_day_) {
            ++Qstr.count;
        }
    }
    if (requests_.back().count > 0) {
        --Qstr.count;
    }
    requests_.push_back(Qstr);
    return result_search;
}

