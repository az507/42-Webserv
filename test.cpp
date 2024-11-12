#include <map>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iterator>

int main(void) {
    std::stringstream ss;
    std::vector<std::string> values;
    std::map<int, std::string> error_pages;

    ss.str("abc 123 456 789");
//    std::copy(std::istream_iterator<std::string>(ss), std::istream_iterator<std::string>(), std::back_inserter(values));
//    std::copy(values.begin(), values.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
    std::vector<std::string>::iterator ite = values.end() - 1;
    (void)ite;

    error_pages[1] = "abc";
    error_pages[2] = "def";
    error_pages[3] = "asd1";
    std::copy(error_pages.begin(), error_pages.end(), std::ostream_iterator<std::pair<int, std::string> >(std::cout));
}
