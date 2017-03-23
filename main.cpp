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

extern "C"
{
    #include "setMem.h"
}

using namespace std;

struct Jobs
{
    int tasks;
};

struct MemoryStruct
{
    char *memory;
    size_t size;
};

struct threadQueue
{
    vector<string> tmp;
    vector<string> orig;
    vector<string> url;
    int threads;
    pthread_mutex_t mutex;
    pthread_cond_t fill;
    int alarm;
    void repopulate()
    {
        tmp = orig;
    }

};

threadQueue producer, consumer, writer;

ofstream outfile;

void my_handler(int sig)
{
    cout << endl << "Caught signal " << strsignal(sig) << endl;
    cout << "Output stored in output.txt" << endl;
    cout << "Exiting..." << endl;
    exit(0);
}

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
    stringstream ss(site);
    string line;

    while(std::getline(ss, line))
    {
        size_t nPos = line.find(target, 0); // fist occurrence
        while(nPos != string::npos)
        {
            total++;
            nPos = line.find(target, nPos+1);
        }
    }
    return total;
}

vector<string> getFileItems(string fileName)
{
    ifstream infile(fileName);
    if(infile.fail())
    {
        cout << "Error with " << fileName << ": " << strerror(errno) << endl;
        exit(1);
    }
    vector<string> searchTerms;

    string temp;
    while (infile >> temp)
        searchTerms.push_back(temp);

    infile.close();

    return searchTerms;
}


void* produce(void * param)
{
    while(producer.tmp.size() > 0)
    {
        pthread_mutex_lock(&producer.mutex);
        string site = producer.tmp.back();
        producer.tmp.pop_back();
        pthread_mutex_unlock(&producer.mutex);

        pthread_mutex_lock(&consumer.mutex);
        string html = getHTMLString(site);

        //        pthread_mutex_lock(&consumer.mutex);
        consumer.tmp.push_back(html);
        consumer.url.push_back(site);
        pthread_cond_signal(&consumer.fill);
        pthread_mutex_unlock(&consumer.mutex);
    }
    return 0;
}

void* consume(void * param)
{
    Jobs *arg_struct = (Jobs*)param;

    for (int i = 0; i < arg_struct->tasks; i++)
    {
        pthread_mutex_lock(&consumer.mutex);
        while (consumer.tmp.size() == 0)
            pthread_cond_wait(&consumer.fill, &consumer.mutex);

        string html = consumer.tmp.back();
        string url = consumer.url.back();
        consumer.tmp.pop_back();
        consumer.url.pop_back();
        pthread_mutex_unlock(&consumer.mutex);

        for (unsigned int i = 0; i < consumer.orig.size(); i++)
        {
            string target = consumer.orig[i];

            long long count = getOccurences(html, target);

            time_t now = time(0);
            string dt = ctime(&now);
            //dt.pop_back();
            dt.erase(dt.size()-1);
            string line;
            line = dt + ":" + target + "," + url + "," + to_string(count) + "\n";

            pthread_mutex_lock(&writer.mutex);
            if (count)
            {
                outfile.open("output.txt", std::ios_base::app);
                if (outfile.fail())
                {
                    cout << "Error with the output file: " << strerror(errno) << endl;
                    exit(1);
                }
                outfile << line;
                outfile.close();
            }
            pthread_mutex_unlock(&writer.mutex);

        }
    }
    return 0;
}

void createThreads(int param)
{
    producer.repopulate();

    pthread_t proThreads[producer.threads];
    pthread_t conThreads[consumer.threads];

    int err;

    for (int i = 0; i < producer.threads; i++)
    {
        err = pthread_create(&proThreads[i], NULL, produce, NULL);
        if (err)
        {
            cout << "Error creating producer threads: " << strerror(errno) << endl;
            exit(1);
        }
    }

    int job = producer.orig.size();

    Jobs jobs[consumer.threads];

    for (int i = 0; i < consumer.threads; i++)
    {
        jobs[i].tasks = job / consumer.threads;

        if (i == consumer.threads - 1)
        {
            jobs[i].tasks += job % consumer.threads;
        }
    }

    for (int i = 0; i < consumer.threads; i++)
    {
        err = pthread_create(&conThreads[i], NULL, consume, &jobs[i]);
        if (err)
        {
            cout << "Error creating consumer threads: " << strerror(errno) << endl;
            exit(1);
        }
    }

    for (int i = 0; i < producer.threads; i++)
    {
        err = pthread_join(proThreads[i], NULL);
        if (err)
        {
            cout << "Error joining producer threads: " << strerror(errno) << endl;
            exit(1);
        }
    }

    for (int i = 0; i < consumer.threads; i++)
    {
        err = pthread_join(conThreads[i], NULL);
        if (err)
        {
            cout << "Error joining consumer threads: " << strerror(errno) << endl;
            exit(1);
        }
    }

    alarm(producer.alarm);
    sleep(producer.alarm);
}

map<string, string> paramCheck(map<string, string> params)
{
    map<string, string> okayParams;
    if (stoi(params["PERIOD_FETCH"]) < 5)
    {
        cout << "PERIOD_FETCH too low, has been set to a default of 5s" << endl;
        okayParams["PERIOD_FETCH"] = "5";
    }
    else
        okayParams["PERIOD_FETCH"] = params["PERIOD_FETCH"];

    if (stoi(params["NUM_FETCH"]) < 1 || stoi(params["NUM_FETCH"]) > 100)
    {
        cout << "NUM_FETCH invalid, has been set to a default of 3" << endl;
        okayParams["NUM_FETCH"] = "3";
    }
    else
        okayParams["NUM_FETCH"] = params["NUM_FETCH"];

    if (stoi(params["NUM_PARSE"]) < 1 || stoi(params["NUM_PARSE"]) > 100)
    {
        cout << "NUM_PARSE invalid, has been set to a default of 3" << endl;
        okayParams["NUM_PARSE"] = "3";
    }
    else
        okayParams["NUM_PARSE"] = params["NUM_PARSE"];

    okayParams["SEARCH_FILE"] = params["SEARCH_FILE"];
    okayParams["SITE_FILE"] = params["SITE_FILE"];

    return okayParams;
}

int main(int argc, char *argv[])
{
    map<string, string> params;
    params["PERIOD_FETCH"] = "10";
    params["NUM_FETCH"] = "1";
    params["NUM_PARSE"] = "1";
    params["SEARCH_FILE"] = "Search.txt";
    params["SITE_FILE"] = "Sites.txt";

    vector<string> pages;

    if (argc > 1)
    {
        string fileName = (string)argv[1];

        ifstream infile(fileName);
        if (infile.fail())
        {
            cout << "Error with params file: " << strerror(errno) << endl;
            exit(1);
        }
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
        infile.close();
    }

    params = paramCheck(params);

    producer.orig = getFileItems(params["SITE_FILE"]);
    producer.threads = stoi(params["NUM_FETCH"]);
    consumer.orig = getFileItems(params["SEARCH_FILE"]);
    consumer.threads = stoi(params["NUM_PARSE"]);
    producer.alarm = stoi(params["PERIOD_FETCH"]);

    struct sigaction sigIntHandler, handler1;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    handler1.sa_handler = my_handler;
    sigemptyset(&handler1.sa_mask);
    handler1.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGHUP, &handler1, NULL);
    signal(SIGALRM, createThreads);
    alarm(producer.alarm);

    createThreads(1);
    sleep(producer.alarm);
    //while(1);
}


