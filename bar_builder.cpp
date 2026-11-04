#include <iostream>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>
#include <functional>
#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <sstream>


class ZeroLengthError : public std::invalid_argument {
public:
    explicit ZeroLengthError(const std::string& mess) :
    std::invalid_argument(mess) {
    }
};
class NotMatchingLengthError : public std::invalid_argument {
public:
    explicit NotMatchingLengthError(const std::string& mess) :
    std::invalid_argument(mess) {
    }
};
class InvalidFieldValue : public std::invalid_argument {
public:
    explicit InvalidFieldValue(const std::string& mess) :
    std::invalid_argument(mess) {
    }
};

template <typename T, typename U=std::string> // 1 – для самих значений, 2 – для labels (строки / не строки)
class Bar {
    static_assert(std::is_arithmetic_v<T>, "Values must be numbers");
private:
    std::vector<std::pair<T, int>> vals; // рядом индекс, чтобы не терялось соответвие инедксам при сортировке
    std::vector<std::pair<double, int>> scaled;

    std::vector<std::pair<double, int>> sorted_vals;

    bool sorted=false;
    bool desc=false;
    char sym='=';
    T scale_coef=10; // приведение к одинаковому масштабу с помощью min-max
    std::map<int,U> labels = {}; // заголовки слева от графика. по умолчанию — индексы
    bool show_qty=true; // показывать ли справа от колонок соответствующее им значение
    bool show_qty_scaled=true; // показывать ли значения колонок в scaled виде (красивше :) )
    std::string title = "-";

    std::vector<std::string> saved_dia;

    std::vector<std::pair<T, int>> proccess_vals(std::vector<T> v) {
        if (v.empty()) {
            throw ZeroLengthError("Please fill in some values. Nothing to display");
        }
        std::vector<std::pair<T, int>> vals;
        for (int i = 0; i < v.size(); i++) {
            vals.push_back({v[i],i+1});
        }
        return vals;
    } 
    static double min_max(T min, T max, T val, int scale_coef) {
        if (min == max) {
            return max * scale_coef;
        }
        return (val - static_cast<double>(min)) / (max - static_cast<double>(min)) * scale_coef;
    }
    std::vector<std::pair<double, int>> scale(std::vector<std::pair<T, int>> v, int scale_coef=10) {
        std::vector<std::pair<double, int>> sc;
        T max_val = v[0].first;
        T min_val = v[0].first;
        for (auto el : v) {
            if (el.first > max_val) {
                max_val = el.first;
            }
            if (el.first < min_val) {
                min_val = el.first;
            }
        }
        for (auto el : v) {
            sc.push_back({min_max(min_val, max_val, el.first, scale_coef),el.second});
        }
        return sc;
    }
    static U expand(size_t len, U val) {
        if constexpr (!std::is_same_v<U, std::string>) {
            return val;
        } else {
            size_t to_add = len - val.length();
            U new_val = val;
            while (to_add > 0) {
                new_val += ' ';
                to_add--;
            }
            return new_val;
        }
    }
    std::map<int,U> set_labels_default(const std::vector<T>& v) {
        std::map<int,U> l;
        int last_ind = v.size();
        for (int i = 1; i <= last_ind; i++) {
            l[i] = stringify(i);
        }
        return l;
    }
    void set_labels_new(std::vector<U> l) {
        size_t max_len = 1;
        if constexpr (std::is_same_v<U, std::string>) {
            for (auto el : l) {
                if (el.length() > max_len) {
                    max_len = el.length();
                }
            }
        }
        for (int i = 0; i < l.size(); i++) {
            labels[i+1] = expand(max_len, l[i]);
        }
    }
    void sort_vals(std::vector<std::pair<double, int>>& v, bool descending) {
        if (!descending) {
            std::sort(v.begin(), v.end(), [](std::pair<double, int> p1, std::pair<double, int> p2) {
                if (p1.first != p2.first) {
                    return p1.first < p2.first;
                }
                return p1.second < p2.second;
            });
        } else {
            std::sort(v.begin(), v.end(), [](std::pair<double, int> p1, std::pair<double, int> p2) {
                if (p1.first != p2.first) {
                    return p1.first > p2.first;
                }
                return p1.second < p2.second;
            });
        }
    }
    template <typename V>
    std::string stringify(const V& val) {
        std::ostringstream stream;
        stream << val;
        return stream.str();
    }
public:
    Bar(std::vector<T> v) :
    vals(proccess_vals(v)) {
        scaled = scale(vals);
        labels = set_labels_default(v);
    }
    Bar& set_sorted(bool val, bool descending=false) {
        sorted = val;
        desc = descending;
        return *this;
    }
    Bar& set_sym(char s) {
        sym = s;
        return *this;
    }
    Bar& set_scale(int sc) {
        if (sc > 300 || sc <= 0) {
            throw InvalidFieldValue("Scale format is invalid, please choose another one");
        }
        scale_coef = sc;
        scaled = scale(vals, sc);
        return *this;
    }
    Bar& set_labels(std::vector<U> l) {
        if (l.size() != vals.size()) {
            throw NotMatchingLengthError("Labels amount shuld be equal to the amount of values");
        }
        set_labels_new(l);
        return *this;
    }
    Bar& set_show_qty(bool val) {
        show_qty = val;
        return *this;
    }
    Bar& set_show_qty_scaled(bool val) {
        show_qty = true;
        show_qty_scaled = val;
        return *this;
    }
    Bar& set_title(std::string t) {
        title = t;
        return *this;
    }

    void build_bars(bool print=true) {
        std::vector<std::string> dia;
        dia.push_back(std::string(3, '<') + " " + title + " " + std::string(3, '>'));
        std::vector<std::pair<double, int>> display = scaled;
        if (sorted) {
            sort_vals(display, desc);
        }
        for (auto bar : display) {
            std::string one_bar = std::string(static_cast<int>(bar.first), sym);
            if (one_bar.empty()) {
                one_bar = ":";
            }
            //std::cout << labels[bar.second] << std::endl;
            std::string b = " " + stringify(labels[bar.second]) + " | " + one_bar + " "; // !
            if (show_qty) {
                if (show_qty_scaled) {
                    b += stringify(static_cast<int>(bar.first));
                } else {
                    auto it = std::find_if(vals.begin(), vals.end(), [bar](std::pair<T, int> x) {
                        return x.second == bar.second;
                    });
                    if (it != vals.end()) {
                        b += stringify((*it).first);
                    }
                }
            }
            dia.push_back(b);
        }
        dia.push_back(std::string('-', scaled.size() * 3));
        saved_dia = dia;
        if (print) {
            for (auto line : dia) {
                std::cout << line << std::endl;
            }
        }
    }
    std::vector<std::string> get_bars() {
        if (!saved_dia.empty()) {
            return saved_dia;
        }
        build_bars(false);
        return saved_dia;
    }
};


int main() {
    Bar bar(std::vector<int>({1,3,3,8,5}));
    bar.set_scale(13).set_sym('$').set_labels({"one","two","three","four","five"}).build_bars();
    Bar b(std::vector<int>({1,3,2,7,10}));
    b.set_title("Custom bar 2").set_show_qty(false).set_show_qty_scaled(false).build_bars();
    Bar b2(std::vector<double>({1.0093,7.34,3.03,9.5,8.99}));
    b2.set_sorted(true, true).set_show_qty_scaled(false).set_sorted(true).build_bars();
    // b2.set_sorted(true, true).set_labels({"A","B","C"}).build_bars(); // error
    // b2.set_scale(999); // error
    
    std::vector<std::string> saved_b = bar.get_bars();
    for (auto line : saved_b) {
        std::cout << line << std::endl;
    }
    return 0;
}