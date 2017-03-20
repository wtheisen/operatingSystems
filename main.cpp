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


    /*static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}



char* getHTML(const char* url)
{
    CURL *curl_handle;
    CURLcode res;

    char* html;
    struct MemoryStruct chunk;

    chunk.memory = (char*)malloc(1);  
    chunk.size = 0;    

    curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, "http://cnn.com/");

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    res = curl_easy_perform(curl_handle);

    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
    else {
        html = chunk.memory;
        cout << "html saved " << html << endl;
        cout << "chunk " << chunk.memory << endl;
        //        printf("%lu bytes retrieved\n", (long)chunk.size);
    }

    curl_easy_cleanup(curl_handle);

    free(chunk.memory);

    curl_global_cleanup();

    return html;
}*/

string getHTMLString(string url1) 
{
    //cout << url1 << endl;
    const char* url = url1.c_str();
    //cout << url << endl;
    char* result = getHTML(url);
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


void* produce(void * param) {

    while(producer.tmp.size() > 0) 
    {
        pthread_mutex_lock(&producer.mutex);
        //while (producer.queue.size() == 0) 
        //{
         //   pthread_cond_wait(&empty, &producer.mutex);
        //} 
        string site = producer.tmp.back();
        producer.tmp.pop_back();
        cout << "site: " << site << endl;
 //       pthread_cond_signal(&fill);
        pthread_mutex_unlock(&producer.mutex);
        

        string html = getHTMLString(site);
        
        pthread_mutex_lock(&consumer.mutex);
        consumer.tmp.push_back(html);
        cout << "html " << consumer.tmp.back() << endl;
        pthread_mutex_unlock(&consumer.mutex);
    }
}

void createProducers(int param) {

    producer.repopulate();

    cout << "okay..." << endl;

    pthread_t threads[producer.threads];
    for (int i = 0; i < producer.threads; i++)
    {  
        pthread_create(&threads[i], NULL, produce, NULL);
    }

    for (int i = 0; i < producer.threads; i++) 
    {
        pthread_join(threads[i], NULL);
    }
    alarm(1);

}

int main(int argc, char *argv[])
{
    map<string, string> params;
    params["PERIOD_FETCH"] = "180";
    params["NUM_FETCH"] = "1";
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
    cout << getHTMLString("http://cnn.com/");
    /*signal(SIGALRM, createProducers);
    alarm(stoi(params["PERIOD_FETCH"]));
    
    createProducers(1);
    while(1);*/
}






