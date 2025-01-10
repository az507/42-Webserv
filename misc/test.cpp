#include <string>
#include <algorithm>
#include <iostream>

std::string trimExtraChars(std::string input, char c) {
    std::string::iterator it, it1, ite;

    it = input.begin();
    ite = input.end();
    while (1) {
        it = std::find(it, ite, c);
        if (it == ite || ++it == ite) {
            break ;
        }
        if (*it == c) {
            it1 = std::find_if(it, ite, std::not1(std::bind2nd(std::equal_to<char>(), c)));
            ite = std::copy(it1, ite, it);
        }
    }
    return std::string(input.begin(), ite);
}

void trimExtraChars1(std::string& input, char c) {
    std::string::iterator it, it1, ite;

    it = input.begin();
    ite = input.end();
    while (1) {
        it = std::find(it, ite, c);
        if (it == ite || ++it == ite) {
            break ;
        }
        if (*it == c) {
            it1 = std::find_if(it, ite, std::not1(std::bind2nd(std::equal_to<char>(), c)));
            ite = std::copy(it1, ite, it);
        }
    }
    input.erase(ite, input.end());
}
  
int main() {
    std::string strs[] = {
        "abc/123/456/789",
        "/this//is///a////test/",
        "//////////////",
        "",
    };

    for (int i = 0; i < 4; ++i) {
        std::cout << "before, strs[" << i << "]: " << strs[i] << std::endl;
        std::cout << "after1, res = " << trimExtraChars(strs[i], '/') << std::endl;
        trimExtraChars1(strs[i], '/');
        std::cout << "after2, strs[" << i << "]: " << strs[i] << std::endl;
    }
}
