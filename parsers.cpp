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
    map<string, string> params;
    params["PERIOD_FETCH"] = "180";
    params["NUM_FETCH"] = "1";
    params["NUM_PARSE"] = "1";
    params["SEARCH_FILE"] = "Search.txt";
    params["SITE_FILE"] = "Sites.txt";

    if (argc > 1)
    {
        string fileName = (string)argv[1];

        ifstream infile(fileName);
        string temp;
        while (infile >> temp)
        {
            string delimiter = "=";
            size_t pos = 0;
            pos = temp.find(delimiter);
            string token = temp.substr(0, pos);
            temp.erase(0, pos + delimiter.length());
            if (params.find(token) != params.end())
                params[token] = temp;
            else
            {
                cout << "Invalid parameter: " << token;
                cout << " with value: " << temp << endl;
            }
        }
    }

    for (auto it = params.begin(); it != params.end(); ++it)
        cout << it->first << " = " << it->second << endl;
}
