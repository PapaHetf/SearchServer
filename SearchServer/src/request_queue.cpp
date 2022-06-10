#include "request_queue.h"

    RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {
    }
    
    vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
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

    vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
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

    int RequestQueue::GetNoResultRequests() const {
        return requests_.back().count;
    }

 