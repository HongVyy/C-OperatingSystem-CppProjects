#include <iostream>
using namespace std;
int main() {
    int n;
    long fact = 1.0;

    cout << "Enter a positive integer: ";
    cin >> n;

    if (n < 0)
        cout << "Error! Factorial of a negative number doesn't exist.";
    else {
        for(int i = 1; i <= n; ++i) {
            fact *= i;
        }
        cout << "The factorial of " << n << " is " << fact;    
    }

    return 0;
}