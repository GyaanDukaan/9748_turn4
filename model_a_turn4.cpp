#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <future>
#include <execution>

template <typename K, typename V>
class ConcurrentHashMap {
public:
    // Insert a new order or update an existing one
    void insert(const K& key, const V& value) {
        std::unique_lock<std::mutex> lock(get_mutex(key));
        auto& orders = map_[key];
        auto it = std::find_if(orders.begin(), orders.end(), [&value](const V& v) {
            return v.price == value.price;
        });
        if (it != orders.end()) {
            it->lotSize += value.lotSize;
            std::cout << "Updated order for key: " << key << " with price: " << value.price << "\n";
        } else {
            orders.push_back(value);
            std::cout << "Inserted new order for key: " << key << " with price: " << value.price << "\n";
        }
    }

    // Update an existing order
    void update(const K& key, int price, const V& newValue) {
        std::unique_lock<std::mutex> lock(get_mutex(key));
        auto it = map_.find(key);
        if (it == map_.end()) {
            std::cerr << "Error: Key not found for update." << std::endl;
            return;
        }
        auto& orders = it->second;
        auto orderIt = std::find_if(orders.begin(), orders.end(), [price](const V& v) {
            return v.price == price;
        });
        if (orderIt != orders.end()) {
            *orderIt = newValue;
            std::cout << "Updated order for key: " << key << " with price: " << price << "\n";
        } else {
            std::cerr << "Error: Order with price not found for update." << std::endl;
        }
    }

    // Remove a key from the map
    void remove(const K& key) {
        std::unique_lock<std::mutex> lock(map_mutex_);
        if (map_.erase(key) == 0) {
            std::cerr << "Error: Key not found for removal." << std::endl;
        }
        mutexes_.erase(key);
    }

    // Display the map
    void display() const {
        std::shared_lock<std::mutex> lock(map_mutex_);
        for (const auto& pair : map_) {
            std::cout << "Key: " << pair.first << "\n";
            for (const auto& value : pair.second) {
                std::cout << "  Price: " << value.price << ", LotSize: " << value.lotSize << "\n";
            }
        }
    }

    // Return the lowest price for a given key
    int get_lowest_price(const K& key) const {
        std::shared_lock<std::mutex> lock(get_mutex(key));
        auto it = map_.find(key);
        if (it == map_.end()) {
            std::cerr << "Error: Key not found for getting lowest price." << std::endl;
            return -1;
        }
        auto& orders = it->second;
        if (orders.empty()) return -1;
        return std::min_element(orders.begin(), orders.end(),
                                [](const V& a, const V& b) { return a.price < b.price; })->price;
    }

    // Return the highest price for a given key
    int get_highest_price(const K& key) const {
        std::shared_lock<std::mutex> lock(get_mutex(key));
        auto it = map_.find(key);
        if (it == map_.end()) {
            std::cerr << "Error: Key not found for getting highest price." << std::endl;
            return -1;
        }
        auto& orders = it->second;
        if (orders.empty()) return -1;
        return std::max_element(orders.begin(), orders.end(),
                                [](const V& a, const V& b) { return a.price < b.price; })->price;
    }

    // Test cases using assert
    void test() {
        insert("NESTLEIND", V(15, 150));
        insert("HDFCBANK", V(20, 1400));
        insert("RELIANCE", V(25, 2500));
        
        insert("NESTLEIND", V(5, 150)); // This should update the existing order
        
        update("NESTLEIND", 150, V(18, 155));
        
        remove("HDFCBANK");
        
        // Asserts to check the state of the map
        assert(map_.at("NESTLEIND")[0].lotSize == 20); // 15 + 5
        assert(map_.at("NESTLEIND")[0].price == 150);
        assert(map_.at("RELIANCE")[0].lotSize == 25);
        assert(map_.at("RELIANCE")[0].price == 2500);
        assert(map_.find("HDFCBANK") == map_.end());
        
        std::cout << "All tests passed!\n";
    }

private:
    std::map<K, std::vector<V>> map_;
    std::map<K, std::mutex> mutexes_;
    mutable std::shared_mutex map_mutex_;
    mutable std::mutex display_mutex_;

    std::mutex& get_mutex(const K& key) {
        std::unique_lock<std::mutex> lock(map_mutex_);
        if (mutexes_.find(key) == mutexes_.end()) {
            mutexes_[key] = std::mutex();
        }
        return mutexes_[key];
    }
};

struct Order {
    int lotSize;
    int price;
    
    Order() : lotSize(10), price(2) {}
    
    Order(int lotSize, int price) : lotSize(lotSize), price(price) {}
    
    bool operator==(const Order& other) const {
        return price == other.price;
    }
};

template <typename K, typename V>
void measure_performance(ConcurrentHashMap<K, V>& map, const K& key, const V& value) {
    auto start = std::chrono::high_resolution_clock::now();
    map.insert(key, value);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "Insert time: " << diff.count() << " seconds\n";

    start = std::chrono::high_resolution_clock::now();
    map.update(key, value.price, value);
    end = std::chrono::high_resolution_clock::now();
    diff = end - start;
    std::cout << "Update time: " << diff.count() << " seconds\n";

    start = std::chrono::high_resolution_clock::now();
    map.remove(key);
    end = std::chrono::high_resolution_clock::now();
    diff = end - start;
    std::cout << "Remove time: " << diff.count() << " seconds\n";
}

int main() {
    ConcurrentHashMap<std::string, Order> concurrentMap;

    // Running test cases
    concurrentMap.test();

    // Display the map
    concurrentMap.display();

    // Measure performance
    measure_performance(concurrentMap, "TESTKEY", Order(10, 100));

    return 0;
}
