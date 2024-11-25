#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <sstream>
#include <chrono>
using namespace std;

class Ingredient;
class Inventory;
class MenuItem;
class Dish;
class Drink;
class User;
class Admin;
class Cart;
class Order;
class Cafe;

void adminMenu(Cafe& cafe);
void userMenu(Cafe& cafe, User* user);
void inventoryMenu(Cafe& cafe);
void budgetMenu(Cafe& cafe);
void menuManagementMenu(Cafe& cafe);
void statisticsMenu(Cafe& cafe);

#pragma region Helpers
string getCurrentDateTime() {
    const auto now = chrono::system_clock::now();
    const auto nowAsTimeT = chrono::system_clock::to_time_t(now);
    struct tm buf;
    localtime_s(&buf, &nowAsTimeT);
    char str[100];
    strftime(str, sizeof(str), "%Y-%m-%d %H:%M", &buf);
    return str;
}

string lowerCase(string str) {
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}
#pragma endregion

class Ingredient {
    string name;
    double quantity;
    string unit;
    double price;
public:
    Ingredient(string name, double price, double quantity, string unit) {
        this->name = name;
        this->price = price;
        this->quantity = quantity;
        this->unit = unit;
    }

    string getName() const { return name; }
    string getUnit() const { return unit; }
    double getPrice() const { return price; }
    double getQuantity() const { return quantity; }

    void setName(string name) { this->name = name; }
    void setUnit(string unit) { this->unit = unit; }

    void setQuantity(double quantity) {
        if (quantity < 0) {
            throw string("Quantity cannot be negative");
        }
        this->quantity = quantity;
    }

    void setPrice(double price) {
        if (price < 0) {
            throw string("Price cannot be negative");
        }
        this->price = price;
    }

    bool decreaseQuantity(double amount) {
        if (quantity >= amount) {
            quantity -= amount;
            return true;
        }
        return false;
    }

    void increaseQuantity(double amount) {
        quantity += amount;
    }
};

class Inventory {
    vector<Ingredient*> ingredients;
public:
    Inventory() {}

    ~Inventory() {
        for (auto* ing : ingredients) {
            delete ing;
        }
    }

    void addIngredient(string name, double price, double quantity, string unit) {
        for (auto* ing : ingredients) {
            if (lowerCase(ing->getName()) == lowerCase(name)) {
                throw string("Ingredient already exists");
            }
        }

        ingredients.push_back(new Ingredient(name, price, quantity, unit));
        saveToFile();
    }

    void removeIngredient(const string& name) {
        for (auto it = ingredients.begin(); it != ingredients.end(); ++it) {
            if (lowerCase((*it)->getName()) == lowerCase(name)) {
                delete* it;
                ingredients.erase(it);
                saveToFile();
                return;
            }
        }
        throw string("Ingredient not found");
    }

    Ingredient* findIngredient(const string& name) {
        for (auto* ing : ingredients) {
            if (lowerCase(ing->getName()) == lowerCase(name)) {
                return ing;
            }
        }
        return nullptr;
    }

    void updateIngredient(const string& name, double newQuantity, double newPrice) {
        auto* ing = findIngredient(name);
        if (!ing) {
            throw string("Ingredient not found");
        }

        double oldCost = ing->getPrice() * ing->getQuantity();
        double newCost = newPrice * newQuantity;
        double costDiff = newCost - oldCost;

        ing->setQuantity(newQuantity);
        ing->setPrice(newPrice);
        saveToFile();
    }

    void loadFromFile() {
        ifstream file("inventory.txt");
        if (!file.is_open()) return;

        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;

            stringstream ss(line);
            string name, unit, priceStr, qtyStr;

            getline(ss, name, ';');
            getline(ss, priceStr, ';');
            getline(ss, qtyStr, ';');
            getline(ss, unit, ';');

            double price = stod(priceStr);
            double quantity = stod(qtyStr);

            ingredients.push_back(new Ingredient(name, price, quantity, unit));
        }
        file.close();
    }

    void saveToFile() {
        ofstream file("inventory.txt");
        if (!file.is_open()) {
            throw string("Cannot open inventory file");
        }

        for (const auto* ing : ingredients) {
            file << ing->getName() << ";"
                << ing->getPrice() << ";"
                << ing->getQuantity() << ";"
                << ing->getUnit() << "\n";
        }
        file.close();
    }

    const vector<Ingredient*>& getIngredients() const {
        return ingredients;
    }
};

class MenuItem {
protected:
    string name;
    double basePrice;
    vector<pair<Ingredient*, double>> ingredients;

public:
    MenuItem(string name, double basePrice) : name(name), basePrice(basePrice) {}
    MenuItem(const MenuItem& other) {
        this->name = other.name;
        this->basePrice = other.basePrice;
    }
    MenuItem& operator=(const MenuItem& other) {
        this->name = other.name;
        this->basePrice = other.basePrice;
    }
    virtual ~MenuItem() {}

    string getName() const { return name; }
    double getBasePrice() const { return basePrice; }
    void setBasePrice(double price) { basePrice = price; }

    void addIngredient(Ingredient* ingredient, double quantity) {
        ingredients.push_back({ ingredient, quantity });
    }

    virtual double calculatePrice() const {
        double total = basePrice;
        for (const auto& pair : ingredients) {
            Ingredient* ing = pair.first;
            double qty = pair.second;
            total += ing->getPrice() * qty;
        }
        return total;
    }

    virtual string getType() const = 0;

    const vector<pair<Ingredient*, double>>& getIngredients() const {
        return ingredients;
    }

    bool updateIngredientQuantity(const string& ingName, double newQty) {
        for (auto& pair : ingredients) {
            if (lowerCase(pair.first->getName()) == lowerCase(ingName)) {
                pair.second = newQty;
                return true;
            }
        }
        return false;
    }

    void showMenuItemIngr() const {
        cout << "\nIngredients are below:\n";
        for (auto& ingr : ingredients) {
            cout << ingr.first->getName() << " - " << ingr.second << "\n";
        }
    }

    /*MenuItem* createModifiedCopy() const {
        MenuItem* copy = (getType() == "Drink") ?
            static_cast<MenuItem*>(new Drink(name, basePrice)) :
            static_cast<MenuItem*>(new Dish(name, basePrice));

        for (const auto& pair : ingredients) {
            copy->addIngredient(pair.first, pair.second);
        }
        return copy;
    }*/
};

class Dish : public MenuItem {
public:
    Dish(string name, double basePrice) : MenuItem(name, basePrice) {}
    string getType() const override { return "Dish"; }
};

class Drink : public MenuItem {
public:
    Drink(string name, double basePrice) : MenuItem(name, basePrice) {}
    string getType() const override { return "Drink"; }
};

class Order {
    static int nextOrderId;
    int orderId;
    string username;
    string datetime;
    vector<pair<MenuItem*, int>> items;
    vector<pair<string, vector<pair<string, double>>>> itemIngredients;
    double totalAmount;

public:
    Order(string username)
        : orderId(++nextOrderId), username(username), datetime(getCurrentDateTime()), totalAmount(0) {}

    void addItem(MenuItem* item, int quantity, const vector<pair<string, double>>& modifiedIngredients = {}) {
        items.push_back({ item, quantity });
        itemIngredients.push_back({ item->getName(), modifiedIngredients.empty() ?
                                getOriginalIngredients(item) : modifiedIngredients });
        totalAmount += item->calculatePrice() * quantity;
    }

    vector<pair<string, double>> getOriginalIngredients(MenuItem* item) {
        vector<pair<string, double>> ingredients;
        for (const auto& pair : item->getIngredients()) {
            ingredients.push_back({ pair.first->getName(), pair.second });
        }
        return ingredients;
    }

    double getTotalAmount() const { return totalAmount; }
    int getOrderId() const { return orderId; }
    const vector<pair<MenuItem*, int>>& getItems() const { return items; }
    const vector<pair<string, vector<pair<string, double>>>>& getItemIngredients() const { return itemIngredients; }

    void saveToFile() const {
        ofstream file("orders.txt", ios::app);
        if (!file.is_open()) {
            throw string("Cannot open orders file");
        }

        file << orderId << ";"
            << username << ";"
            << datetime << ";"
            << totalAmount << "\n";

        ofstream detailFile("order_details.txt", ios::app);
        if (!detailFile.is_open()) {
            throw string("Cannot open order details file");
        }

        for (size_t i = 0; i < items.size(); i++) {
            detailFile << orderId << ";"
                << items[i].first->getName() << ";"
                << items[i].second << ";"
                << items[i].first->calculatePrice() << ";";

            const auto& ingredients = itemIngredients[i].second;
            for (size_t j = 0; j < ingredients.size(); j++) {
                detailFile << ingredients[j].first << ":" << ingredients[j].second;
                if (j < ingredients.size() - 1) detailFile << ",";
            }
            detailFile << "\n";
        }

        file.close();
        detailFile.close();
    }
};

int Order::nextOrderId = 0;

class Cart {
    vector<pair<MenuItem*, int>> items;
    vector<MenuItem*> modifiedItems;
    double total;
public:
    Cart() : total(0) {}
    ~Cart() {
        for (auto* item : modifiedItems) {
            delete item;
        }
    }

    void addItem(MenuItem* item, int quantity) {
        /*MenuItem* modifiedItem = item->createModifiedCopy();*/
        MenuItem* copy = item;

        modifiedItems.push_back(copy);
        items.push_back({ copy, quantity });
        recalculateTotal();
    }

    void removeItem(const string& itemName) {
        for (auto it = items.begin(); it != items.end(); ++it) {
            if (it->first->getName() == itemName) {
                for (auto modIt = modifiedItems.begin(); modIt != modifiedItems.end(); ++modIt) {
                    if ((*modIt)->getName() == itemName) {
                        delete* modIt;
                        modifiedItems.erase(modIt);
                        break;
                    }
                }
                items.erase(it);
                recalculateTotal();
                return;
            }
        }
    }

    bool modifyItemIngredient(const string& itemName, const string& ingName, double newQty) {
        for (auto& pair : items) {
            if (pair.first->getName() == itemName) {
                auto ingredients = pair.first->getIngredients();
                if (ingredients.size() == 1 && newQty == 0) {
                    throw string("Cannot remove the only ingredient from item");
                }

                bool found = false;
                for (const auto& ingPair : ingredients) {
                    if (ingPair.first->getName() == ingName) {
                        found = true;
                        if (newQty > ingPair.first->getQuantity()) {
                            throw string("Insufficient ingredient quantity in inventory");
                        }
                        break;
                    }
                }

                if (!found) {
                    throw string("Ingredient not found in item");
                }

                pair.first->updateIngredientQuantity(ingName, newQty);
                recalculateTotal();
                return true;
            }
        }
        return false;
    }

    void recalculateTotal() {
        total = 0;
        for (const auto& pair : items) {
            total += pair.first->calculatePrice() * pair.second;
        }
    }

    double getTotal() const { return total; }
    const vector<pair<MenuItem*, int>>& getItems() const { return items; }

    void clear() {
        for (auto* item : modifiedItems) {
            delete item;
        }
        modifiedItems.clear();
        items.clear();
        total = 0;
    }
};

class User {
    string username;
    string password;
    vector<Order*> orderHistory;
    Cart* cart;

public:
    User(string username, string password)
        : username(username), password(password) {
        cart = new Cart();
    }

    ~User() {
        delete cart;
        for (auto* order : orderHistory) {
            delete order;
        }
    }

    static bool validatePassword(const string& pwd) {
        if (pwd.length() < 6) return false;
        for (char c : pwd) {
            if (!isalnum(c)) return false;
        }
        return true;
    }

    bool checkPassword(const string& pwd) const { return password == pwd; }
    string getUsername() const { return username; }
    string getPassword() const { return password; }
    Cart* getCart() { return cart; }

    void addToOrderHistory(Order* order) {
        orderHistory.push_back(order);
    }

    const vector<Order*>& getOrderHistory() const {
        return orderHistory;
    }
};

class Admin {
    string username;
    string password;

public:
    Admin(string username, string password) : username(username), password(password) {}
    bool authenticate(const string& user, const string& pwd) const {
        return username == user && password == pwd;
    }
    ~Admin() {}
};

class Cafe {
    double budget;
    Inventory* inventory;
    vector<User*> users;
    vector<MenuItem*> menuItems;
    Admin* admin;

public:
    Cafe(double initialBudget) : budget(initialBudget) {
        admin = new Admin("admin", "admin123");
        inventory = new Inventory();
        loadData();
    }

    ~Cafe() {
        delete admin;
        delete inventory;
        for (auto* user : users) delete user;
        for (auto* item : menuItems) delete item;
    }

    bool updateBudget(double amount) {
        if (budget + amount < 0) return false;
        budget += amount;
        saveBudgetToFile();
        return true;
    }

    void registerUser(const string& username, const string& password) {
        if (username.length() > 30) {
            throw string("Username is too long");
        }

        for (const auto* user : users) {
            if (user->getUsername() == username) {
                throw string("Username already exists");
            }
        }

        if (!User::validatePassword(password)) {
            throw string("Invalid password format");
        }

        users.push_back(new User(username, password));
        saveUsersToFile();
    }

    User* login(const string& username, const string& password) {
        for (auto* user : users) {
            if (user->getUsername() == username && user->checkPassword(password)) {
                return user;
            }
        }
        return nullptr;
    }

    bool adminLogin(const string& username, const string& password) {
        return admin->authenticate(username, password);
    }

    void addMenuItem(const string& name, double basePrice, bool isDrink) {
        for (const auto* item : menuItems) {
            if (item->getName() == name) {
                throw string("Menu item already exists");
            }
        }

        MenuItem* newItem = isDrink ?
            static_cast<MenuItem*>(new Drink(name, basePrice)) :
            static_cast<MenuItem*>(new Dish(name, basePrice));

        menuItems.push_back(newItem);
        saveMenuToFile();
    }

    void removeMenuItem(const string& name) {
        for (auto it = menuItems.begin(); it != menuItems.end(); ++it) {
            if ((*it)->getName() == name) {
                delete* it;
                menuItems.erase(it);
                saveMenuToFile();
                return;
            }
        }
        throw string("Menu item not found");
    }

    MenuItem* findMenuItem(const string& name) {
        for (auto* item : menuItems) {
            if (item->getName() == name) {
                return item;
            }
        }
        return nullptr;
    }

    Order* processOrder(User* user) {
        Cart* cart = user->getCart();
        if (cart->getItems().empty()) {
            throw string("Cart is empty");
        }

        for (const auto& cartItem : cart->getItems()) {
            MenuItem* item = cartItem.first;
            int itemQty = cartItem.second;

            for (const auto& ingPair : item->getIngredients()) {
                Ingredient* ing = ingPair.first;
                double ingQty = ingPair.second;
                if (ing->getQuantity() < ingQty * itemQty) {
                    throw string("Not enough " + ing->getName() + " in stock");
                }
            }
        }

        Order* order = new Order(user->getUsername());
        for (const auto& cartItem : cart->getItems()) {
            MenuItem* item = cartItem.first;
            int qty = cartItem.second;

            vector<pair<string, double>> modifiedIngredients;
            for (const auto& ingPair : item->getIngredients()) {
                modifiedIngredients.push_back({ ingPair.first->getName(), ingPair.second });
                ingPair.first->decreaseQuantity(ingPair.second * qty);
            }

            order->addItem(item, qty, modifiedIngredients);
        }

        updateBudget(order->getTotalAmount());
        order->saveToFile();
        user->addToOrderHistory(order);
        saveStatistics(order);
        cart->clear();
        inventory->saveToFile();

        return order;
    }

    void loadData() {
        loadBudgetFromFile();
        inventory->loadFromFile();
        loadUsersFromFile();
        loadMenuFromFile();
    }

    void loadBudgetFromFile() {
        ifstream file("budget.txt");
        if (!file.is_open()) return;

        string line;
        getline(file, line);
        if (!line.empty()) {
            budget = stod(line);
        }
        file.close();
    }

    void saveBudgetToFile() {
        ofstream file("budget.txt");
        if (!file.is_open()) {
            throw string("Cannot open budget file");
        }
        file << budget << "\n";
        file.close();
    }

    void loadUsersFromFile() {
        ifstream file("users.txt");
        if (!file.is_open()) return;

        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string username, password;
            getline(ss, username, ';');
            getline(ss, password, ';');
            users.push_back(new User(username, password));
        }
        file.close();
    }

    void saveUsersToFile() {
        ofstream file("users.txt");
        if (!file.is_open()) {
            throw string("Cannot open users file");
        }
        for (const auto* user : users) {
            file << user->getUsername() << ";" << user->getPassword() << "\n";
        }
        file.close();
    }

    void loadMenuFromFile() {
        ifstream file("menu.txt");
        if (!file.is_open()) return;

        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string name, type, priceStr;

            getline(ss, name, ';');
            getline(ss, priceStr, ';');
            getline(ss, type, ';');

            double basePrice = stod(priceStr);

            if (type == "Drink") {
                menuItems.push_back(new Drink(name, basePrice));
            }
            else {
                menuItems.push_back(new Dish(name, basePrice));
            }
        }
        file.close();

        loadMenuIngredients();
    }

    void loadMenuIngredients() {
        ifstream file("menu_ingredients.txt");
        if (!file.is_open()) return;

        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string itemName, ingName, qty;

            getline(ss, itemName, ';');
            MenuItem* item = findMenuItem(itemName);
            if (!item) continue;

            while (getline(ss, ingName, ';') && getline(ss, qty, ';')) {
                Ingredient* ing = inventory->findIngredient(ingName);

                if (ing) {
                    item->addIngredient(ing, stod(qty));
                }
            }
           /* getline(ss, itemName, ';');
            getline(ss, ingName, ';');
            getline(ss, qtyStr, ';');

            double qty = stod(qtyStr);

            MenuItem* item = findMenuItem(itemName);
            Ingredient* ing = inventory->findIngredient(ingName);

            if (item && ing) {
                item->addIngredient(ing, qty);
            }*/
        }
        file.close();
    }

    void saveMenuToFile() {
        ofstream file("menu.txt");
        if (!file.is_open()) {
            throw string("Cannot open menu file");
        }

        /*ofstream ingFile("menu_ingredients.txt");
        if (!ingFile.is_open()) {
            throw string("Cannot open menu ingredients file");
        }*/

        for (const auto* item : menuItems) {
            file << item->getName() << ";"
                << item->getBasePrice() << ";"
                << item->getType() << "\n";

            /*for (const auto& pair : item->getIngredients()) {
                ingFile << item->getName() << ";"
                    << pair.first->getName() << ";"
                    << pair.second << "\n";
            }*/
        }
        file.close();
        //ingFile.close();
    }

    void saveMenuItemIngredientsToFile(const string& itemName, const vector<pair<Ingredient*, double>>& ingredients) {
        ofstream ingFile("menu_ingredients.txt", ios::app);
        if (!ingFile.is_open()) {
            throw string("Cannot open menu ingredients file");
        }

        try {
            ingFile << itemName;
            for (const auto& pair : ingredients) {
                Ingredient* ing = pair.first;
                double qty = pair.second;

                ingFile << ";" << ing->getName() << ";" << qty;
            }
            ingFile << "\n";
            ingFile.close();
        }
        catch (const string& error) {
            ingFile.close();
            throw;
        }
    }

    struct DailySale {
        string date;
        double amount;
    };

    vector<DailySale> getWeeklySales(const string& startDate) {
        vector<DailySale> sales;
        ifstream file("daily_stats.txt");
        if (!file.is_open()) return sales;

        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string date, amountStr;
            getline(ss, date, ';');
            getline(ss, amountStr, ';');

            if (date >= startDate && date < getNextWeekDate(startDate)) {
                sales.push_back({ date, stod(amountStr) });
            }
        }
        file.close();
        return sales;
    }

    string getNextWeekDate(const string& date) {
        tm timeinfo = {};
        istringstream ss(date);
        ss >> get_time(&timeinfo, "%Y-%m-%d");
        timeinfo.tm_mday += 7;
        mktime(&timeinfo);
        char buffer[11];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
        return string(buffer);
    }

    void saveStatistics(const Order* order) {
        ofstream daily("daily_stats.txt", ios::app);
        if (!daily.is_open()) {
            throw string("Cannot open daily stats file");
        }
        daily << getCurrentDateTime().substr(0, 10) << ";"
            << order->getTotalAmount() << "\n";
        daily.close();
    }

    double getBudget() const { return budget; }
    Inventory* getInventory() { return inventory; }
    const vector<MenuItem*>& getMenu() const { return menuItems; }
};

void showWeeklySales(Cafe& cafe) {
    cout << "\n=== Weekly Sales ===\n";
    ifstream file("daily_stats.txt");
    if (!file.is_open()) {
        cout << "No sales data available\n";
        return;
    }

    string line;
    vector<pair<string, double>> allSales;

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string date, amountStr;

        getline(ss, date, ';');
        getline(ss, amountStr, ';');

        allSales.push_back({ date, stod(amountStr) });
    }
    file.close();

    if (allSales.empty()) {
        cout << "No sales data available\n";
        return;
    }

    sort(allSales.begin(), allSales.end());
    string currentWeekStart = allSales[0].first;
    double weeklyTotal = 0;

    for (const auto& sale : allSales) {
        if (sale.first >= cafe.getNextWeekDate(currentWeekStart)) {
            cout << "\nTotal for week starting " << currentWeekStart << ": $" << weeklyTotal << "\n\n";
            currentWeekStart = sale.first;
            weeklyTotal = 0;
        }
        cout << sale.first << ": $" << sale.second << "\n";
        weeklyTotal += sale.second;
    }
    cout << "\nTotal for week starting " << currentWeekStart << ": $" << weeklyTotal << "\n";
}

void menuManagementMenu(Cafe& cafe) {
    int choice;
    do {
        cout << "\n=== Menu Management ===\n"
            << "1. Add Menu Item\n"
            << "2. Remove Menu Item\n"
            << "3. Update Menu Item\n"
            << "4. View Menu\n"
            << "0. Back\n"
            << "Choice: ";

        cin >> choice;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        try {
            switch (choice) {
            case 1: {
                string name;
                double price;
                int type;

                cout << "Enter item name: ";
                getline(cin, name);
                cout << "Enter base price: $";
                cin >> price;
                cout << "Type (1 for Dish, 2 for Drink): ";
                cin >> type;
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cafe.addMenuItem(name, price, type == 2);
                MenuItem* item = cafe.findMenuItem(name);

                char addMore;
                do {
                    string ingName;
                    double qty;

                    cout << "Add ingredient:\n";
                    cout << "Ingredient name: ";
                    getline(cin, ingName);
                    cout << "Quantity needed: ";
                    cin >> qty;
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');

                    Ingredient* ing = cafe.getInventory()->findIngredient(ingName);
                    if (!ing) {
                        throw string("Ingredient not found!");
                    }
                    
                    item->addIngredient(ing, qty);
                    item->showMenuItemIngr();
                    

                    cout << "Add another ingredient? (y/n): ";
                    cin >> addMore;
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                } while (tolower(addMore) == 'y');
                cafe.saveMenuItemIngredientsToFile(item->getName(), item->getIngredients());
                break;
            }

            case 2: {
                string name;
                cout << "Enter item name to remove: ";
                getline(cin, name);
                cafe.removeMenuItem(name);
                break;
            }

            case 3: {
                string name;
                cout << "Enter item name to update: ";
                getline(cin, name);

                MenuItem* item = cafe.findMenuItem(name);
                if (!item) {
                    throw string("Menu item not found!");
                }

                int updateChoice;
                cout << "\n1. Update ingredients\n"
                    << "2. Update base price\n"
                    << "Choice: ";
                cin >> updateChoice;

                if (updateChoice == 1) {
                    string ingName;
                    double qty;
                    cout << "Enter ingredient name: ";
                    cin.ignore();
                    getline(cin, ingName);
                    cout << "Enter new quantity: ";
                    cin >> qty;

                    if (!item->updateIngredientQuantity(ingName, qty)) {
                        throw string("Ingredient not found in menu item!");
                    }
                }
                else if (updateChoice == 2) {
                    double newPrice;
                    cout << "Enter new base price: $";
                    cin >> newPrice;
                    item->setBasePrice(newPrice);
                }
                cafe.saveMenuToFile();
                break;
            }

            case 4: {
                cout << "\n=== Current Menu ===\n";
                for (const auto* item : cafe.getMenu()) {
                    cout << "\n" << item->getType() << ": " << item->getName()
                        << "\nBase Price: $" << item->getBasePrice()
                        << "\nIngredients:\n";

                    for (const auto& pair : item->getIngredients()) {
                        cout << "- " << pair.first->getName() << ": " << pair.second
                            << " " << pair.first->getUnit() << endl;
                    }
                    cout << "Total Price: $" << item->calculatePrice() << "\n";
                }
                break;
            }

            case 0:
                break;

            default:
                cout << "Invalid choice!\n";
            }
        }
        catch (const string& error) {
            cout << "Error: " << error << endl;
        }
    } while (choice != 0);
}

void statisticsMenu(Cafe& cafe) {
    int choice;
    do {
        cout << "\n=== Statistics ===\n"
            << "1. Daily Sales\n"
            << "2. Weekly Sales\n"
            << "0. Back\n"
            << "Choice: ";

        cin >> choice;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        try {
            switch (choice) {
            case 1: {
                cout << "\n=== Daily Sales ===\n";
                ifstream file("daily_stats.txt");
                if (!file.is_open()) {
                    cout << "No sales data available\n";
                    break;
                }

                string line;
                vector<pair<string, double>> dailySales;

                while (getline(file, line)) {
                    if (line.empty()) continue;
                    stringstream ss(line);
                    string date, amountStr;

                    getline(ss, date, ';');
                    getline(ss, amountStr, ';');

                    bool found = false;
                    for (auto& sale : dailySales) {
                        if (sale.first == date) {
                            sale.second += stod(amountStr);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        dailySales.push_back({ date, stod(amountStr) });
                    }
                }
                file.close();

                sort(dailySales.begin(), dailySales.end());
                for (const auto& sale : dailySales) {
                    cout << sale.first << ": $" << sale.second << endl;
                }
                break;
            }
            case 2:
                showWeeklySales(cafe);
                break;

            case 0:
                break;

            default:
                cout << "Invalid choice!\n";
            }
        }
        catch (const string& error) {
            cout << "Error: " << error << endl;
        }
    } while (choice != 0);
}

void userMenu(Cafe& cafe, User* user) {
    int choice;
    do {
        cout << "\n=== Welcome, " << user->getUsername() << "! ===\n"
            << "1. View Menu\n"
            << "2. Place Order\n"
            << "3. View Cart\n"
            << "4. Modify Cart Item\n"
            << "5. View Order History\n"
            << "0. Logout\n"
            << "Choice: ";

        cin >> choice;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        try {
            switch (choice) {
            case 1: {
                cout << "\n=== Menu ===\n";
                for (const auto* item : cafe.getMenu()) {
                    cout << "\n" << item->getType() << ": " << item->getName()
                        << "\nPrice: $" << item->calculatePrice()
                        << "\nIngredients:\n";
                    for (const auto& pair : item->getIngredients()) {
                        cout << "- " << pair.first->getName() << ": " << pair.second
                            << " " << pair.first->getUnit() << endl;
                    }
                }
                break;
            }

            case 2: {
                string itemName;
                int quantity;

                cout << "Enter item name: ";
                getline(cin, itemName);
                cout << "Enter quantity: ";
                cin >> quantity;

                MenuItem* item = cafe.findMenuItem(itemName);
                if (!item) {
                    throw string("Menu item not found!");
                }

                if (cafe.getBudget() < cafe.findMenuItem(itemName)->getBasePrice() * quantity) {
                    throw string("Can't add due to budgetary restrictions");
                    //convert to try catch block along with the code below until break
                }
                user->getCart()->addItem(item, quantity);
                cout << "Item added to cart!\n";
                break;
            }

            case 3: {
                Cart* cart = user->getCart();
                cout << "\n=== Your Cart ===\n";
                for (const auto& cartItem : cart->getItems()) {
                    cout << cartItem.first->getName() << " x" << cartItem.second << "\n";
                    cout << "Ingredients:\n";
                    for (const auto& ing : cartItem.first->getIngredients()) {
                        cout << "- " << ing.first->getName() << ": " << ing.second
                            << " " << ing.first->getUnit() << endl;
                    }
                    cout << "Price: $" << cartItem.first->calculatePrice() * cartItem.second << endl;
                }
                cout << "Total: $" << cart->getTotal() << endl;

                if (!cart->getItems().empty()) {
                    char checkout;
                    cout << "\nProceed to checkout? (y/n): ";
                    cin >> checkout;

                    if (tolower(checkout) == 'y') {
                        Order* order = cafe.processOrder(user);
                        cout << "Order placed successfully!\n"
                            << "Total amount: $" << order->getTotalAmount() << endl;
                    }
                }
                break;
            }

            case 4: {
                Cart* cart = user->getCart();
                if (cart->getItems().empty()) {
                    cout << "Cart is empty!\n";
                    break;
                }

                string itemName;
                cout << "Enter item name to modify: ";
                getline(cin, itemName);

                string ingName;
                cout << "Enter ingredient name to modify: ";
                getline(cin, ingName);

                double newQty;
                cout << "Enter new quantity: ";
                cin >> newQty;

                if (cafe.getBudget() < cafe.findMenuItem(itemName)->getBasePrice() * newQty) {
                    throw string("Can't add due to budgetary restrictions");
                    //convert to try catch block along with the code below until break
                }
                cart->modifyItemIngredient(itemName, ingName, newQty);
                cout << "Item modified successfully!\n";
                break;
            }

            case 5: {
                cout << "\n=== Order History ===\n";
                for (const auto* order : user->getOrderHistory()) {
                    cout << "\nOrder #" << order->getOrderId()
                        << " - Total: $" << order->getTotalAmount() << endl;

                    for (const auto& itemPair : order->getItems()) {
                        cout << "\n" << itemPair.first->getName()
                            << " x" << itemPair.second << endl;
                        cout << "Used ingredients:\n";
                        for (const auto& ingInfo : order->getItemIngredients()) {
                            if (ingInfo.first == itemPair.first->getName()) {
                                for (const auto& ing : ingInfo.second) {
                                    cout << "- " << ing.first << ": " << ing.second << endl;
                                }
                            }
                        }
                    }
                }
                break;
            }

            case 0:
                cout << "Logging out...\n";
                break;

            default:
                cout << "Invalid choice!\n";
            }
        }
        catch (const string& error) {
            cout << "Error: " << error << endl;
        }
    } while (choice != 0);
}

void adminMenu(Cafe& cafe) {
    int choice;
    do {
        cout << "\n=== Admin Menu ===\n"
            << "1. Inventory Management\n"
            << "2. Budget Management\n"
            << "3. Menu Management\n"
            << "4. Statistics\n"
            << "0. Logout\n"
            << "Choice: ";

        cin >> choice;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        try {
            switch (choice) {
            case 1:
                inventoryMenu(cafe);
                break;
            case 2:
                budgetMenu(cafe);
                break;
            case 3:
                menuManagementMenu(cafe);
                break;
            case 4:
                statisticsMenu(cafe);
                break;
            case 0:
                cout << "Logging out...\n";
                break;
            default:
                cout << "Invalid choice!\n";
            }
        }
        catch (const string& error) {
            cout << "Error: " << error << endl;
        }
    } while (choice != 0);
}

void inventoryMenu(Cafe& cafe) {
    int choice;
    do {
        cout << "\n=== Inventory Management ===\n"
            << "1. Add Ingredient\n"
            << "2. Remove Ingredient\n"
            << "3. Update Ingredient\n"
            << "4. View Inventory\n"
            << "0. Back\n"
            << "Choice: ";

        cin >> choice;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        try {
            string name;
            double price, quantity;
            string unit;

            switch (choice) {
            case 1:
                cout << "Name: ";
                getline(cin, name);
                do {
                    cout << "\nPrice can NOT be negative:\n";
                    cout << "Price: ";
                    cin >> price;
                } while (price < 0);
                do {
                    cout << "\nQuantity can NOT be negative:\n";
                    cout << "Quantity: ";
                    cin >> quantity;
                } while (quantity < 0);
                if (cafe.getBudget() < price * quantity) {
                    throw string("Can't add due to budgetary restrictions");
                    //convert to try catch block along with the code below until break
                }
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Unit: ";
                getline(cin, unit);

                cafe.getInventory()->addIngredient(name, price, quantity, unit);
                cout << "Ingredient added successfully!\n";
                break;

            case 2:
                cout << "Enter ingredient name to remove: ";
                getline(cin, name);
                cafe.getInventory()->removeIngredient(name);
                cout << "Ingredient removed successfully!\n";
                break;

            case 3:
                cout << "Enter ingredient name to update: ";
                getline(cin, name);
                do {
                    cout << "\nPrice can NOT be negative:\n";
                    cout << "Price: ";
                    cin >> price;
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                } while (price < 0);
                do {
                    cout << "\nQuantity can NOT be negative:\n";
                    cout << "Quantity: ";
                    cin >> quantity;
                } while (quantity < 0);
                if (cafe.getBudget() < price * quantity) {
                    throw string("Can't add due to budgetary restrictions");
                    //convert to try catch block along with the code below until break
                }
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                cafe.getInventory()->updateIngredient(name, quantity, price);
                cout << "Ingredient updated successfully!\n";
                break;

            case 4:
                cout << "\n=== Current Inventory ===\n";
                for (const auto* ing : cafe.getInventory()->getIngredients()) {
                    cout << ing->getName() << ": "
                        << ing->getQuantity() << " " << ing->getUnit()
                        << " (Price: $" << ing->getPrice() << ")\n";
                }
                break;

            case 0:
                break;

            default:
                cout << "Invalid choice!\n";
            }
        }
        catch (const string& error) {
            cout << "Error: " << error << endl;
        }
    } while (choice != 0);
}

void budgetMenu(Cafe& cafe) {
    int choice;
    do {
        cout << "\n=== Budget Management ===\n"
            << "Current Budget: $" << cafe.getBudget() << endl
            << "1. Add Funds\n"
            << "2. Withdraw Funds\n"
            << "0. Back\n"
            << "Choice: ";

        cin >> choice;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        try {
            double amount;
            switch (choice) {
            case 1:
                cout << "Enter amount to add: $";
                cin >> amount;
                if (cafe.updateBudget(amount)) {
                    cout << "Budget updated successfully!\n";
                }
                else {
                    cout << "Failed to update budget!\n";
                }
                break;

            case 2:
                cout << "Enter amount to withdraw: $";
                cin >> amount;
                if (cafe.updateBudget(-amount)) {
                    cout << "Budget updated successfully!\n";
                }
                else {
                    cout << "Insufficient funds!\n";
                }
                break;

            case 0:
                break;

            default:
                cout << "Invalid choice!\n";
            }
        }
        catch (const string& error) {
            cout << "Error: " << error << endl;
        }
    } while (choice != 0);
}

void mainMenu(Cafe& cafe) {
    int choice;
    do {
        cout << "\n=== Welcome to Cafe Azure ===\n"
            << "1. Admin Login\n"
            << "2. User Registration\n"
            << "3. User Login\n"
            << "0. Exit\n"
            << "Choice: ";

        cin >> choice;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        try {
            switch (choice) {
            case 1: {
                string username, password;
                cout << "Username: ";
                getline(cin, username);
                cout << "Password: ";
                getline(cin, password);

                if (cafe.adminLogin(username, password)) {
                    adminMenu(cafe);
                }
                else {
                    cout << "Invalid credentials!\n";
                }
                break;
            }
            case 2: {
                string username, password;
                cout << "Enter new username: ";
                getline(cin, username);
                cout << "Enter password (min 6 chars, letters and numbers only): ";
                getline(cin, password);

                cafe.registerUser(username, password);
                cout << "Registration successful!\n";
                break;
            }
            case 3: {
                string username, password;
                cout << "Username: ";
                getline(cin, username);
                cout << "Password: ";
                getline(cin, password);

                User* user = cafe.login(username, password);
                if (user) {
                    userMenu(cafe, user);
                }
                else {
                    cout << "Invalid credentials!\n";
                }
                break;
            }
            case 0:
                cout << "Thank you for visiting Cafe Azure!\n";
                break;
            default:
                cout << "Invalid choice!\n";
            }
        }
        catch (const string& error) {
            cout << "Error: " << error << endl;
        }
    } while (choice != 0);
}

void main() {
   try {
        Cafe cafe(10000.0);
        mainMenu(cafe);
    }
    catch (const string& error) {
        cout << "Error: " << error << endl;
    }
}