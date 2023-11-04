#include<iostream>
using namespace std;

int main(){
	int i, j, rows, columns;
	char ch;
	cout << "\nPlease the Number of Rows    =  ";
	cin >> rows;
	cout << "\nEnter the Number of Columns =  ";
	cin >> columns;
	cout << "\nEnter Symbol to Print  =  ";
	cin >> ch;	
	cout << "\n-----Rectangle Pattern-----\n";

	while(i < rows){
    	j = 0; 
        while(j < columns){
           cout << ch;
           j++;
        }
        cout << "\n";
        i++;
	}
 	return 0;
}