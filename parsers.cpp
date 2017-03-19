#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <iterator>
#include <sstream>
using namespace std;

vector<string> getFileItems(string fileName)
{
    ifstream infile(fileName);
    vector<string> searchTerms;

    string temp;
    while (infile >> temp)
        searchTerms.push_back(temp);

    for (auto it = searchTerms.begin(); it != searchTerms.end(); ++it)
        cout << *it << endl;

    return searchTerms;
}

int main(int argc, char *argv[])
{
    vector<string> meow = getFileItems(argv[1]);
}
