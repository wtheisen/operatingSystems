//Operating Systems project 4
//Michael Burke and William Theisen

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

extern "C" //Includes the c file for use in C++
{
    #include "setMem.h"
}

using namespace std;

struct Jobs //this struct is used as the argument for the producer threads
{
    int tasks;
};

struct MemoryStruct //structure for storing the HTML in memory
{
    char *memory;
    size_t size;
};

struct threadQueue //All of our threads share this generic structure for their data
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

threadQueue producer, consumer, writer; //the three types of threads we use in our code

ofstream outfile; //the output file we dump to

void my_handler(int sig) //the handler to catch the interrupt signal
{
    cout << endl << "Caught signal " << strsignal(sig) << endl;
    cout << "Output stored in output.txt" << endl;
    cout << "Exiting..." << endl;
    exit(0);
}

string getHTMLString(string url1) //gets the HTML string
{
    char * S = new char[url1.length() + 1];
    strcpy(S,url1.c_str());
    char* result = getHTML(S);
    return result;
}

int getOccurences(string site, string target) //find the occurences of target in site
{
    int total = 0;
    stringstream ss(site); //convert site to a stream
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

vector<string> getFileItems(string fileName) //reads in the items from file line by line
{                                            //used for Site.txt and Searches.txt
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


void* produce(void * param) //the produce portion of the code, fetches the HTML string for the consumers
{                           //and places it in a queue(vector) for them
    while(producer.tmp.size() > 0)
    {
        pthread_mutex_lock(&producer.mutex); //lock so no two consumers get the same URL
        string site = producer.tmp.back();
        producer.tmp.pop_back();
        pthread_mutex_unlock(&producer.mutex);

        pthread_mutex_lock(&consumer.mutex); //lock so we don't try and write to the queue
        string html = getHTMLString(site);   //at the same time

        consumer.tmp.push_back(html);
        consumer.url.push_back(site);
        pthread_cond_signal(&consumer.fill);
        pthread_mutex_unlock(&consumer.mutex);
    }
    return 0;
}

void* consume(void * param) //the consume portion of the code, takes an HTMLstring out of the queue
{                           //and parses it, then writes to the output file
    Jobs *arg_struct = (Jobs*)param;

    for (int i = 0; i < arg_struct->tasks; i++)
    {
        pthread_mutex_lock(&consumer.mutex); //lock so no two consumers get the same string
        while (consumer.tmp.size() == 0)
            pthread_cond_wait(&consumer.fill, &consumer.mutex);

        string html = consumer.tmp.back();
        string url = consumer.url.back();
        consumer.tmp.pop_back();
        consumer.url.pop_back();
        pthread_mutex_unlock(&consumer.mutex);

        for (unsigned int i = 0; i < consumer.orig.size(); i++) //this goes through and gets all the occurences
        {                                                       //for all the words
            string target = consumer.orig[i];

            long long count = getOccurences(html, target);

            time_t now = time(0);
            string dt = ctime(&now);
            //dt.pop_back();
            dt.erase(dt.size()-1);
            string line;
            line = dt + ":" + target + "," + url + "," + to_string(count) + "\n"; //prepare the output string

            pthread_mutex_lock(&writer.mutex); //lock so we don't try and write to the output file at the same time
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
    producer.repopulate(); //fill the producer queue with the URLS every roung

    pthread_t proThreads[producer.threads]; //create two arrays for the two sets of threads
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
        jobs[i].tasks = job / consumer.threads; //math to make sure the consumers will do the right amount of work

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

    cout << "Outputting..." << endl; //just a nice message to let the user know the code is running

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

    alarm(producer.alarm); //set the alarm for the next cycle
    sleep(producer.alarm); //after work is done go to sleep until the next cycle
                           //sleeping makes the idle between rounds more efficient
}

map<string, string> paramCheck(map<string, string> params) //this function sanity checks the options in
{                                                          //the user provided config file
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
    map<string, string> params; //set up our default parameters
    params["PERIOD_FETCH"] = "10";
    params["NUM_FETCH"] = "1";
    params["NUM_PARSE"] = "1";
    params["SEARCH_FILE"] = "Search.txt";
    params["SITE_FILE"] = "Sites.txt";

    vector<string> pages;

    if (argc > 1) //this if statement will read in the provided config file
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

    params = paramCheck(params); //check the parameters

    producer.orig = getFileItems(params["SITE_FILE"]); //set up the thread structs
    producer.threads = stoi(params["NUM_FETCH"]);
    consumer.orig = getFileItems(params["SEARCH_FILE"]);
    consumer.threads = stoi(params["NUM_PARSE"]);
    producer.alarm = stoi(params["PERIOD_FETCH"]);

    struct sigaction sigIntHandler, handler1; //prepare the sig handler

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

    createThreads(1); //create the threads for the first round
    sleep(producer.alarm); //after the first round returns sleep
}


