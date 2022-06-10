#pragma once

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) {
        _begin = begin;
        _end = end;
    }
    auto begin() const {
        return _begin;
    }
    auto end() const {
        return _end;
    }
    size_t size() const {
        return _end - _begin;
    }
private:
    Iterator _begin;
    Iterator _end;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        size_t size = end - begin;
        if (size % page_size == 0) {
            for (auto it = begin; it < end; it += page_size) {
                vector_page.push_back(IteratorRange(it, it + page_size));
            }
        }
        else if ((size % page_size != 0) && size > page_size) {
            auto it = begin;
            size_t temp_i = 0;
            for (size_t i = 0; i < size / page_size; ++i) {
                vector_page.push_back(IteratorRange(it, it + page_size));
                it += page_size;
                temp_i += page_size;
            }
            vector_page.push_back(IteratorRange(it, it + (size - temp_i)));
        }
    }
    auto begin() const {
        return vector_page.begin();
    }
    auto end() const {
        return vector_page.end();
    }
    auto size() {
        return vector_page.end() - vector_page.begin();
    }

private:
    std::vector<IteratorRange<Iterator>> vector_page;//вектор диапазонов 
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& it) {
    for (auto it_begin = it.begin(); it_begin != it.end(); ++it_begin) {
        out << *it_begin;
    }
    return out;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
