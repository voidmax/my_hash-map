// Copyright (c) 2020, Vladimir Romanov <vrom272000@gmail.com>
// All rights reserved.
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <memory>
#include <utility>

// The realisation of hash table with linear iteration using open addressing
// with linear probing search strategy. 

// Let n be the current number of elements in the hash table.
// It will be used in formulas of the complexity.

// The size of the table is at most n * kDensity^3 and at least n * kDensity.
// If this statement is false, Rebuild will set the size of the table to
// the geometric mean of those bounds.
// In other words, n * kSizeChange = n * kDensity^2.

template<class KeyType, class ValueType,
        class Hash = std::hash<KeyType>>
class HashMap {
public:
    static const size_t kNoValue = SIZE_MAX;
    static const size_t kDeleted = SIZE_MAX - 1;
    static const size_t kDensity = 2;
    static const size_t kSizeChange = kDensity * kDensity;

    using Element = std::pair<const KeyType, ValueType>;

    class iterator {
    private:
        HashMap* owner_;
        size_t pointer_;

    public:
        iterator() {}

        iterator(HashMap* _owner, size_t it) : owner_(_owner), pointer_(it) {}

        iterator(const iterator& other) {
            owner_ = other.owner_;
            pointer_ = other.pointer_;
        }

        iterator& operator++() {
            ++pointer_;
            return *this;
        }

        iterator operator++(int) {
            auto old_pointer = pointer_++;
            return iterator(owner_, old_pointer);
        }
        bool operator == (iterator other) const {
            return other.pointer_ == pointer_ && other.owner_ == owner_;
        }

        bool operator != (iterator other) const {
            return !(*this == other);
        }

        Element& operator *() {
            return *(owner_->elements_[pointer_]);
        }

        Element* operator ->() {
            return &(operator*());
        }
    };

    class const_iterator {
    private:
        const HashMap* owner_;
        size_t pointer_;

    public:
        const_iterator() {}

        const_iterator(const HashMap* _owner, size_t it) : owner_(_owner),
                                                           pointer_(it) {}

        const_iterator& operator++() {
            ++pointer_;
            return *this;
        }

        const_iterator(const const_iterator& other) {
            owner_ = other.owner_;
            pointer_ = other.pointer_;
        }

        const_iterator operator++(int) {
            auto old_pointer = pointer_++;
            return const_iterator(owner_, old_pointer);
        }

        bool operator == (const_iterator other) const {
            return other.pointer_ == pointer_;
        }

        bool operator != (const_iterator other) const {
            return other.pointer_ != pointer_;
        }

        const Element& operator *() const {
            return *(owner_->elements_[pointer_]);
        }

        const Element* operator ->() const {
            return &(operator*());
        }
    };

    iterator begin() {
        return iterator(this, 0);
    }

    iterator end() {
        return iterator(this, elements_.size());
    }

    const_iterator begin() const {
        return const_iterator(this, 0);
    }

    const_iterator end() const {
        return const_iterator(this, elements_.size());
    }

    explicit HashMap(Hash _hasher = Hash()) : hasher_(_hasher) {
        Rebuild();
    }

    // The expected time complexity is O(n).
    // The worst-case time complexity is O(n^2).
    template <class ForwardIt>
    HashMap(ForwardIt begin,
            ForwardIt end,
            Hash _hasher = Hash()) : hasher_(_hasher) {
        hasher_ = _hasher;
        Rebuild();
        for (auto it = begin; it != end; ++it) {
            insert(*it);
        }
    }

    // The expected time complexity is O(n).
    // The worst-case time complexity is O(n^2).
    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list,
            Hash _hasher = Hash()) : hasher_(_hasher) {
        hasher_ = _hasher;
        Rebuild();
        for (auto it = list.begin(); it != list.end(); ++it) {
            insert(*it);
        }
    }

    // The expected time complexity is O(n).
    // The worst-case time complexity is O(n^2).
    HashMap(const HashMap& other) : hasher_(other.hasher_) {
        Rebuild();
        for (auto element : other) {
            insert(element);
        }
    }

    // The expected time complexity is O(n).
    // The worst-case time complexity is O(n^2).
    HashMap& operator = (HashMap other) {
        clear();
        for (auto element : other) {
            insert(element);
        }
        return *this;
    }

    // The time complexity is O(1).
    size_t size() const {
        return elements_.size();
    }

    // The time complexity is O(1).
    bool empty() const {
        return elements_.empty();
    }

    // The time complexity is O(1).
    Hash hash_function() const {
        return hasher_;
    }

    // The expected time complexity is O(1).
    // The worst-case time complexity is O(n).
    void insert(const std::pair<KeyType, ValueType>& value) {
        size_t id = FindPlace(value.first);
        if (place_[id] == kNoValue) {
            AddElement(id, value);
        }
    }

    // The expected time complexity is O(1).
    // The worst-case time complexity is O(n).
    void erase(const KeyType& key) {
        size_t id = FindPlace(key);
        if (place_[id] != kNoValue) {
            DeleteElement(id);
        }
    }

    // The expected time complexity is O(1).
    // The worst-case time complexity is O(n).
    iterator find(const KeyType& key) {
        size_t id = FindPlace(key);
        if (place_[id] == kNoValue) {
            return end();
        }
        return iterator(this, place_[id]);
    }

    // The expected time complexity is O(1).
    // The worst-case time complexity is O(n).
    const_iterator find(const KeyType& key) const {
        size_t id = FindPlace(key);
        if (place_[id] == kNoValue) {
            return end();
        }
        return const_iterator(this, place_[id]);
    }

    // The expected time complexity is O(1).
    // The worst-case time complexity is O(n).
    ValueType& operator[](const KeyType& key) {
        size_t id = FindPlace(key);
        if (place_[id] == kNoValue) {
            AddElement(id, {key, ValueType()});
            id = FindPlace(key);  // Id may be changed after Rebuild.
        }
        return elements_[place_[id]]->second;
    }

    // The expected time complexity is O(1).
    // The worst-case time complexity is O(n).
    const ValueType& at(const KeyType& key) const {
        size_t id = FindPlace(key);
        if (place_[id] == kNoValue) {
            throw std::out_of_range("There is no element");
        }
        return elements_[place_[id]]->second;
    }

    // The complexity is O(n).
    void clear() {
        elements_.clear();
        Rebuild();
    }

private:
    size_t operations_complete_ = 0;
    Hash hasher_;
    std::vector<std::unique_ptr<Element>> elements_;
    std::vector<size_t> place_;
    std::vector<size_t> rev_place_;

    // This method changes the structure of the table in order to maintain some
    // acceptable density.
    // Iterators and pointers can be invalidated.
    // The expected time complexity is O(n).
    // The worst-case time complexity is O(n^2).
    void Rebuild() {
        if (elements_.empty()) {
            place_.resize(1);
        } else {
            place_.resize(elements_.size() * kSizeChange);
        }
        rev_place_.resize(place_.size());
        fill(place_.begin(), place_.end(), kNoValue);
        operations_complete_ = 0;

        std::vector<std::unique_ptr<Element>> temp;
        for (auto& i : elements_) {
            temp.emplace_back(i.release());
        }
        elements_.clear();

        for (const auto& element : temp) {
            insert(*element);
        }
    }

    // This method finds the place where the key should be.
    // The expected time complexity is O(1).
    // The worst-case time complexity is O(n).
    size_t FindPlace(const KeyType& key) const {
        size_t i = hasher_(key) % place_.size();
        while (true) {
            if (i == place_.size()) {
                i = 0;
            }
            if (place_[i] == kNoValue ||
                (place_[i] != kDeleted && elements_[place_[i]]->first == key)) {
                return i;
            }
            ++i;
        }
    }

    // This method changes the size of the array place_ if it is too big.
    // The expected time complexity is O(1).
    // The worst-case time complexity is O(n).
    void MakeGoodBig() {
        if (operations_complete_ * kDensity >= place_.size()) {
            Rebuild();
        }
    }

    // This method changes the size of the array place_ if it is too small.
    // The expected time complexity is O(1).
    // The worst-case time complexity is O(n).
    void MakeGoodSmall() {
        if (place_.size() > elements_.size() * kSizeChange * kDensity) {
            Rebuild();
        }
    }

    // This method adds the given element assuming at place id assuming that
    // place was found using method FindPlace.
    // The expected time complexity is O(1).
    // The worst-case time complexity is O(n).
    void AddElement(size_t id, const Element& value) {
        ++operations_complete_;
        place_[id] = elements_.size();
        rev_place_[elements_.size()] = id;
        elements_.emplace_back(new Element(value));
        MakeGoodBig();
    }

    // This method deletes the element at place id assuming that place id is not
    // empty.
    // The expected time complexity is O(1).
    // The worst-case time complexity is O(n).
    void DeleteElement(size_t id) {
        std::swap(elements_.back(), elements_[place_[id]]);
        elements_.pop_back();
        place_[rev_place_[elements_.size()]] = place_[id];
        rev_place_[place_[id]] = rev_place_[elements_.size()];
        place_[id] = kDeleted;
        MakeGoodSmall();
    }
};

template<class KeyType, class ValueType, class Hash>
const size_t HashMap<KeyType, ValueType, Hash>::kNoValue;

template<class KeyType, class ValueType, class Hash>
const size_t HashMap<KeyType, ValueType, Hash>::kDeleted;

template<class KeyType, class ValueType, class Hash>
const size_t HashMap<KeyType, ValueType, Hash>::kDensity;

template<class KeyType, class ValueType, class Hash>
const size_t HashMap<KeyType, ValueType, Hash>::kSizeChange;
