// StockTracker.cpp
#include "StockTracker.h"
#include "httplib.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include "imgui/imgui.h"
#include <iostream>
#include <iomanip>
#include <sstream>

const std::string API_KEY = "0L1BTNVU82R4JSP1"; 
const std::string API_HOST = "www.alphavantage.co";

StockTracker::StockTracker() 
    : running_(false), 
      showFavoritesOnly_(false),
      selectedSymbol_("") {
    
    memset(searchBuffer_, 0, sizeof(searchBuffer_));
    
    stocks_["AAPL"] = Stock();
    stocks_["AAPL"].symbol = "AAPL";
    stocks_["AAPL"].name = "Apple Inc.";
    
    stocks_["GOOGL"] = Stock();
    stocks_["GOOGL"].symbol = "GOOGL";
    stocks_["GOOGL"].name = "Alphabet Inc.";
    
    stocks_["MSFT"] = Stock();
    stocks_["MSFT"].symbol = "MSFT";
    stocks_["MSFT"].name = "Microsoft Corporation";
    
    stocks_["TSLA"] = Stock();
    stocks_["TSLA"].symbol = "TSLA";
    stocks_["TSLA"].name = "Tesla Inc.";
    
    stocks_["AMZN"] = Stock();
    stocks_["AMZN"].symbol = "AMZN";
    stocks_["AMZN"].name = "Amazon.com Inc.";
    
    loadFavorites();
}

StockTracker::~StockTracker() {
    stop();
}

void StockTracker::start() {
    running_ = true;
    updateThread_ = std::thread(&StockTracker::updateLoop, this);
}

void StockTracker::stop() {
    running_ = false;
    if (updateThread_.joinable()) {
        updateThread_.join();
    }
}

void StockTracker::updateLoop() {
    while (running_) {
        updateAllStocks();
        
        for (int i = 0; i < 60 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

nlohmann::json StockTracker::fetchStockData(const std::string& symbol) {
    try {
        httplib::SSLClient client(API_HOST);
        client.set_connection_timeout(5, 0);
        client.set_read_timeout(5, 0);
        
        std::string path = "/query?function=GLOBAL_QUOTE&symbol=" + 
                          symbol + "&apikey=" + API_KEY;
        
        auto res = client.Get(path.c_str());
        
        if (res && res->status == 200) {
            return nlohmann::json::parse(res->body);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error fetching data for " << symbol << ": " 
                  << e.what() << std::endl;
    }
    
    return nlohmann::json();
}

void StockTracker::updateAllStocks() {
    for (auto& [symbol, stock] : stocks_) {
        auto data = fetchStockData(symbol);
        
        if (!data.empty() && data.contains("Global Quote")) {
            std::lock_guard<std::mutex> lock(dataMutex_);
            
            auto quote = data["Global Quote"];
            
            if (quote.contains("05. price")) {
                stock.price = std::stod(quote["05. price"].get<std::string>());
            }
            
            if (quote.contains("09. change")) {
                stock.change = std::stod(quote["09. change"].get<std::string>());
            }
            
            if (quote.contains("10. change percent")) {
                std::string changeStr = quote["10. change percent"].get<std::string>();
                changeStr.erase(std::remove(changeStr.begin(), changeStr.end(), '%'), 
                               changeStr.end());
                stock.changePercent = std::stod(changeStr);
            }
            
          
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time), "%H:%M:%S");
            stock.lastUpdate = ss.str();
        }
        
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void StockTracker::loadFavorites() {
    std::ifstream file("favorites.json");
    if (file.is_open()) {
        try {
            nlohmann::json j;
            file >> j;
            
            if (j.contains("favorites")) {
                favoriteSymbols_ = j["favorites"].get<std::vector<std::string>>();
                
               
                for (const auto& symbol : favoriteSymbols_) {
                    if (stocks_.find(symbol) != stocks_.end()) {
                        stocks_[symbol].isFavorite = true;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading favorites: " << e.what() << std::endl;
        }
        file.close();
    }
}

void StockTracker::saveFavorites() {
    nlohmann::json j;
    j["favorites"] = favoriteSymbols_;
    
    std::ofstream file("favorites.json");
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
    }
}

void StockTracker::toggleFavorite(const std::string& symbol) {
    auto it = std::find(favoriteSymbols_.begin(), favoriteSymbols_.end(), symbol);
    
    if (it != favoriteSymbols_.end()) {
        favoriteSymbols_.erase(it);
        if (stocks_.find(symbol) != stocks_.end()) {
            stocks_[symbol].isFavorite = false;
        }
    } else {
        favoriteSymbols_.push_back(symbol);
        if (stocks_.find(symbol) != stocks_.end()) {
            stocks_[symbol].isFavorite = true;
        }
    }
    
    saveFavorites();
}

void StockTracker::renderUI() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    
    ImGui::Begin("Stock Market Tracker", nullptr, 
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                 ImGuiWindowFlags_NoCollapse);
    
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::Text("📈 Stock Market Tracker");
    ImGui::PopFont();
    ImGui::Separator();
   
    ImGui::InputText("Search", searchBuffer_, sizeof(searchBuffer_));
    ImGui::SameLine();
    ImGui::Checkbox("Favorites Only", &showFavoritesOnly_);
    
    ImGui::Separator();
    
   
    if (ImGui::BeginTable("StocksTable", 7, 
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable)) {
        
        ImGui::TableSetupColumn("⭐", ImGuiTableColumnFlags_WidthFixed, 30);
        ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Change", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Change %", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Last Update", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableHeadersRow();
        
        for (const auto& [symbol, stock] : stocks_) {
            std::string search(searchBuffer_);
            if (!search.empty() && 
                symbol.find(search) == std::string::npos &&
                stock.name.find(search) == std::string::npos) {
                continue;
            }
            
            // סינון לפי מועדפים
            if (showFavoritesOnly_ && !stock.isFavorite) {
                continue;
            }
            
            ImGui::TableNextRow();
            
            
            ImGui::TableNextColumn();
            std::string btnLabel = stock.isFavorite ? "★##" : "☆##";
            btnLabel += symbol;
            if (ImGui::SmallButton(btnLabel.c_str())) {
                const_cast<StockTracker*>(this)->toggleFavorite(symbol);
            }
            
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", symbol.c_str());
            
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", stock.name.c_str());
            
           
            ImGui::TableNextColumn();
            ImGui::Text("$%.2f", stock.price);
            
         
            ImGui::TableNextColumn();
            if (stock.change >= 0) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "+$%.2f", stock.change);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "$%.2f", stock.change);
            }
            
      
            ImGui::TableNextColumn();
            if (stock.changePercent >= 0) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "+%.2f%%", stock.changePercent);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.2f%%", stock.changePercent);
            }
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", stock.lastUpdate.c_str());
        }
        
        ImGui::EndTable();
    }
    
    ImGui::Separator();
    ImGui::Text("Status: %s", running_.load() ? "🟢 Active" : "🔴 Stopped");
    ImGui::SameLine(ImGui::GetWindowWidth() - 200);
    ImGui::Text("Total Stocks: %d", (int)stocks_.size());
    
    ImGui::End();
}