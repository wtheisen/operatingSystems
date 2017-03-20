#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <iterator> 

extern "C" {
#include "setMem.h"
}

using namespace std;

struct MemoryStruct {
    char *memory;
    size_t size;
};


struct threadQueue {

    vector<string> tmp;
    vector<string> orig;
    int threads;
    pthread_mutex_t mutex;
    void repopulate() {
        tmp = orig;
    }

};

threadQueue producer, consumer; 

string getHTMLString(string url1) 
{
    char * S = new char[url1.length() + 1];
    strcpy(S,url1.c_str());
    char* result = getHTML(S);
    return result;
}

int getOccurences(string site, string target) 
{
    int total = 0;
    std::stringstream ss(site);
    std::string line;
    int count = 0;
    while(std::getline(ss, line))
    {
        size_t nPos = line.find(target, 0); // fist occurrence
        while(nPos != string::npos)
        {
            total++;
            nPos = line.find(target, nPos+1);
        }
       // cout << "total: " << total << endl;
        count++;
    }

    return total;
}

vector<string> getFileItems(string fileName)
{
    ifstream infile(fileName);
    vector<string> searchTerms;

    string temp;
    while (infile >> temp)
        searchTerms.push_back(temp);

    //for (auto it = searchTerms.begin(); it != searchTerms.end(); ++it)
    //       cout << *it << endl;

    return searchTerms;
}


void* produce(void * param) 
{
    while(producer.tmp.size() > 0) 
    {
        pthread_mutex_lock(&producer.mutex);
        string site = producer.tmp.back();
        producer.tmp.pop_back();
        //cout << "site: " << site << endl;
        pthread_mutex_unlock(&producer.mutex);
        

        string html = getHTMLString(site);
        
        pthread_mutex_lock(&consumer.mutex);
        consumer.tmp.push_back(html);
        pthread_cond_signal(&consumer.fill, &consumer.mutex);
        //cout << "html " << consumer.tmp.back() << endl;
        pthread_mutex_unlock(&consumer.mutex);
    }
}

void* consumer(void * param)
{

    Jobs *arg_struct = param;

    for (int i = 0; i < arg_struct->tasks; i++)
    {
        pthread_mutex_lock(&consumer.mutex);
        while (consumer.tmp.size() == 0)
            //do conditional wait thing
        string HTML = consumer.tmp.back();
        consumer.tmp.pop_back();
        pthread_cond_wait(&consumer.fill, &consumer.mutex);
        // singal here?
        pthread_mutex_unlock(&consumer.mutex);

        string target = "test";
        int count = getOccurences(HTML, target);
        cout << count << endl;
        
        
        //pthread_mutex_lock(&consumer.mutex);
    }
}

void createProducers(int param) {

    producer.repopulate();

    cout << "okay..." << endl;

    pthread_t proThreads[producer.threads];
    //pthread_t conThreads[consumer.threads];
    pthread_t conThreads[consumer.threads];
    for (int i = 0; i < producer.threads; i++)
    {  
        pthread_create(&proThreads[i], NULL, produce, NULL);
    }

    int job = producer.orig.size();

    struct Jobs {
        int tasks;
    };
    
    Jobs jobs[consumer.threads];

    for (int i = 0; i < consumer.threads; i++) {
        jobs[i].tasks = job / consumer.threads;
        if (i == consumer.threads - 1)
        {
            jobs[i].tasks += job % consumer.threads;
        }
    }

    for (int i = 0; i < consumer.threads; i ++)
    {
        pthread_create(&conThreads[i], NULL, consume, &jobs[i]);
    }
 
    for (int i = 0; i < producer.threads; i++) 
    {
        pthread_join(proThreads[i], NULL);
    }
 
    for (int i = 0; i < consumer.threads; i++)
    {
        pthread_join(conThreads[i], NULL);
    }
    
    //int currThreads = 0;
    //int totThreads = 0;
    
    /*while (totThreads < producer.orig.size())
    {
        pthread_create(&conThreads[totThreads], NULL, consume, NULL);
        currThreads++; totThreads++;
        if (currThreads == consumer.threads)
        {
            pthread_join(conThreads[totThreads - (consumer.threads - 1)], NULL);
            currThreads--;
        }
    }
    for (int i = 0; i < totThreads; i++)
    {
        pthread_join(conThreads[i], NULL);
    }*/
    
    // hard coded value of 180
    alarm(180);

}

int main(int argc, char *argv[])
{
    map<string, string> params;
    params["PERIOD_FETCH"] = "180";
    params["NUM_FETCH"] = "8";
    params["NUM_PARSE"] = "1";
    params["SEARCH_FILE"] = "Search.txt";
    params["SITE_FILE"] = "Sites.txt";

    vector<string> pages;

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

    producer.orig = getFileItems(params["SITE_FILE"]);
    producer.threads = stoi(params["NUM_FETCH"]);
    consumer.orig = getFileItems(params["SEARCH_FILE"]);
    consumer.threads = stoi(params["NUM_PARSE"]);

 //cout << site << endl;
    //cout << getHTMLString("http://cnn.com/");
    signal(SIGALRM, createProducers);
    alarm(stoi(params["PERIOD_FETCH"]));
    
    createProducers(1);
    while(1);
}


