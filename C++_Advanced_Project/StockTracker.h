// StockTracker.h
#ifndef STOCK_TRACKER_H
#define STOCK_TRACKER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>


struct Stock {
    std::string symbol;       
    std::string name;         
    double price;              
    double change;             
    double changePercent;      
    std::string lastUpdate;    
    bool isFavorite;           
    
    Stock() : price(0.0), change(0.0), changePercent(0.0), isFavorite(false) {}
};

class StockTracker {
public:
    StockTracker();
    ~StockTracker();
    

    void start();
    
  
    void stop();
    
   
    void renderUI();

private:
    
    std::unordered_map<std::string, Stock> stocks_;
    std::vector<std::string> favoriteSymbols_;
    
    std::thread updateThread_;
    std::atomic<bool> running_;
    std::mutex dataMutex_;
    
    char searchBuffer_[64];
    std::string selectedSymbol_;
    bool showFavoritesOnly_;
    
    void updateLoop();
    
    nlohmann::json fetchStockData(const std::string& symbol);
    
   
    void updateAllStocks();
    
    void loadFavorites();
   
    void saveFavorites();
    
    void toggleFavorite(const std::string& symbol);
};

#endif // STOCK_TRACKER_H