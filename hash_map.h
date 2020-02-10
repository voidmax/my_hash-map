// Copyright [2020] <voidmax>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <memory>
#include <utility>

template<class KeyType, class ValueType,
        class Hash = std::hash<KeyType>>
class HashMap {
    using Info = std::pair<const KeyType, ValueType>;

    size_t NO_VALUE = SIZE_MAX;
    size_t DELETED = SIZE_MAX - 1;
    size_t SIZE_LIMIT = 2;

    size_t operations_complete = 0;
    Hash hasher;
    std::vector<std::shared_ptr<Info>> elements;
    std::vector<size_t> place, rev_place;

    void rebuild() {
        place.resize(elements.size() * (SIZE_LIMIT * 2) + 1);
        rev_place.resize(place.size());
        fill(place.begin(), place.end(), NO_VALUE);
        operations_complete = 0;

        std::vector<std::shared_ptr<Info>> temp;
        for (auto i : elements) {
            temp.push_back(i);
        }
        elements.clear();

        for (auto element : temp) {
            insert(*element);
        }
    }

    size_t find_place(const KeyType& key) const {
        for (size_t i = hasher(key) % place.size();; ++i) {
            if (i == place.size()) {
                i = 0;
            }
            if (place[i] == NO_VALUE ||
                (place[i] != DELETED && elements[place[i]]->first == key)) {
                return i;
            }
        }
    }

    void adding(size_t id, const Info& value) {
        ++operations_complete;
        place[id] = elements.size();
        rev_place[elements.size()] = id;
        elements.emplace_back(std::shared_ptr<Info>(new Info(value)));
        if (operations_complete * SIZE_LIMIT >= place.size()) {
            rebuild();
        }
    }

    void deleting(size_t id) {
        std::swap(elements.back(), elements[place[id]]);
        elements.pop_back();
        place[rev_place[elements.size()]] = place[id];
        rev_place[place[id]] = rev_place[elements.size()];
        place[id] = DELETED;
    }

 public:
    class iterator {
    private:
        HashMap* owner;
        size_t pointer;

    public:
        iterator() {}

        iterator(HashMap* _owner, size_t it) : owner(_owner), pointer(it) {}

        iterator(const iterator& other) {
            owner = other.owner;
            pointer = other.pointer;
        }

        iterator& operator++() {
            ++pointer;
            return *this;
        }

        iterator operator++(int) {
            auto old_pointer = pointer++;
            return iterator(owner, old_pointer);
        }
        bool operator == (iterator other) const {
            return other.pointer == pointer && other.owner == owner;
        }

        bool operator != (iterator other) const {
            return !(*this == other);
        }

        Info& operator *() {
            return *(owner->elements[pointer]);
        }

        Info* operator ->() {
            return &(operator*());
        }
    };

    class const_iterator {
    private:
        const HashMap* owner;
        size_t pointer;

    public:
        const_iterator() {}

        const_iterator(const HashMap* _owner, size_t it) : owner(_owner),
                                                           pointer(it) {}

        const_iterator& operator++() {
            ++pointer;
            return *this;
        }

        const_iterator(const const_iterator& other) {
            owner = other.owner;
            pointer = other.pointer;
        }

        const_iterator operator++(int) {
            auto old_pointer = pointer++;
            return const_iterator(owner, old_pointer);
        }

        bool operator == (const_iterator other) const {
            return other.pointer == pointer;
        }

        bool operator != (const_iterator other) const {
            return other.pointer != pointer;
        }

        const Info& operator *() const {
            return *(owner->elements[pointer]);
        }

        const Info* operator ->() const {
            return &(operator*());
        }
    };

    iterator begin() {
        return iterator(this, 0);
    }

    iterator end() {
        return iterator(this, elements.size());
    }

    const_iterator begin() const {
        return const_iterator(this, 0);
    }

    const_iterator end() const {
        return const_iterator(this, elements.size());
    }

    explicit HashMap(Hash _hasher = Hash()) : hasher(_hasher) {
        rebuild();
    }

    template <class ForwardIt>
    HashMap(ForwardIt begin,
            ForwardIt end,
            Hash _hasher = Hash()) : hasher(_hasher) {
        hasher = _hasher;
        rebuild();
        for (auto it = begin; it != end; ++it) {
            insert(*it);
        }
    }

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list,
            Hash _hasher = Hash()) : hasher(_hasher) {
        hasher = _hasher;
        rebuild();
        for (auto it = list.begin(); it != list.end(); ++it) {
            insert(*it);
        }
    }

    HashMap(const HashMap& other) : hasher(other.hasher) {
        rebuild();
        for (auto element : other) {
            insert(element);
        }
    }

    HashMap& operator = (HashMap other) {
        clear();
        for (auto element : other) {
            insert(element);
        }
        return *this;
    }

    size_t size() const {
        return elements.size();
    }

    bool empty() const {
        return elements.empty();
    }

    Hash hash_function() const {
        return hasher;
    }

    void insert(const std::pair<KeyType, ValueType>& value) {
        size_t id = find_place(value.first);
        if (place[id] == NO_VALUE) {
            adding(id, value);
        }
    }

    void erase(const KeyType& key) {
        size_t id = find_place(key);
        if (place[id] != NO_VALUE) {
            deleting(id);
        }
    }

    iterator find(const KeyType& key) {
        size_t id = find_place(key);
        if (place[id] == NO_VALUE) {
            return end();
        }
        return iterator(this, place[id]);
    }

    const_iterator find(const KeyType& key) const {
        size_t id = find_place(key);
        if (place[id] == NO_VALUE) {
            return end();
        }
        return const_iterator(this, place[id]);
    }

    ValueType& operator[](const KeyType& key) {
        size_t id = find_place(key);
        if (place[id] == NO_VALUE) {
            adding(id, {key, ValueType()});
            id = find_place(key);  // id may be changed after rebuild
        }
        return elements[place[id]]->second;
    }

    const ValueType& at(const KeyType& key) const {
        size_t id = find_place(key);
        if (place[id] == NO_VALUE) {
            throw std::out_of_range("There is no element");
        }
        return elements[place[id]]->second;
    }

    void clear() {
        elements.clear();
        rebuild();
    }
};
