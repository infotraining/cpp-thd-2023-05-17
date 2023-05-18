#include "thread_safe_lookup_table.hpp"

#include <iostream>
#include <string>
#include <thread>

using namespace std;

int main()
{
    cout << "lookup-table" << endl;

    ThreadSafeLookupTable<int, std::string> lookup_table;
    lookup_table.add_or_update_mapping(42, "forty two");

    std::thread thd_write {[&]()
        {
            lookup_table.add_or_update_mapping(1, "one");
            lookup_table.add_or_update_mapping(7, "seven");
            
            for (int i = 10; i < 1000; ++i)
                lookup_table.add_or_update_mapping(i, "item_"s + std::to_string(i));

            for (int i = 10; i < 1000; i+=2)
                lookup_table.add_or_update_mapping(i, "element_"s + std::to_string(i));
        }
    };

    std::thread thd_reader1 {[&]()
        {
            std::cout << "1: " << lookup_table.value_for(1) << std::endl;
            std::cout << "7: " << lookup_table.value_for(1) << std::endl;
            std::cout << "42: " << lookup_table.value_for(1) << std::endl;

            for(int i = 100; i < 1000; ++i)
                std::cout << lookup_table.value_for(i) << std::endl;
        }
    };

    std::thread thd_reader2 {[&]()
        {
            std::cout << "1: " << lookup_table.value_for(1) << std::endl;
            std::cout << "7: " << lookup_table.value_for(1) << std::endl;
            std::cout << "42: " << lookup_table.value_for(1) << std::endl;

            for(int i = 100; i < 1000; ++i)
                std::cout << lookup_table.value_for(i) << std::endl;
        }
    };

    thd_write.join();
    thd_reader1.join();
    thd_reader2.join();
}
