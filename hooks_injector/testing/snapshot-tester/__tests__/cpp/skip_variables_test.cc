#include <iostream>
#include <string>

class SimpleTest {
public:
    void TestMethod(int param1, const std::string& param2) {
        int localVar1 = 42;
        std::string localVar2 = "test";
        double localVar3 = 3.14;
        
        if (param1 > 0) {
            int nestedVar = localVar1 * 2;
            localVar2 = param2 + "_modified";
        }
        
        std::cout << "Method completed" << std::endl;
    }
    
    int GetValue() {
        int result = 100;
        return result;
    }
};

void GlobalFunction(bool flag) {
    int counter = 0;
    if (flag) {
        counter++;
    }
}
