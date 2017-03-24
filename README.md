# <center>University of Notre Dame</center>
#### Principles of Operating Systems
#### Project 4
#### Michael Burke and William Theisen

---

This project will continually poll a list of URLs to see if they contain
any items in the Search.txt file. It will then output matches to
the system terminal. It does so using *multi-threading*.

---

It works by first getting the parameters, either reading in from a file
provided as the first argument of the prorgam, or if a file is not
specified it runs using a set of defaults. Then the program creates two sets
of threads, one to go fetch the HTML from a set of sites, and the second
to parse the HTML for the search terms as specified in the file. The program
has 5 configuration parameters that can be specified in a text file to be
passed in:

PERIOD_FETCH: The time in seconds between the running of the fetch threads
NUM_FETCH: The number of fetch threads we wish to run (1 < x < 100)
NUM_PARSE: The number of consumer threads to use (1 < x < 100)
SEARCH_FILE: The file containing the strings to search from
SITE_FILE: The file containing the URLs to parse

If the parameters do not meet the standards they will be set to a default
value and if any of the files does not exist the program will output an error
and exit. The program continues to run until an interrupt is caught at which
point it will exit. It exits almost immediately meaning that some data might
be lost. The output dumping works as follows: after the consume threads have
parsed their specific HTML-string, they lock and append the output to a file
named for the particular fetch period.

Included in the testFiles directory is a Sites.txt and a Searches.txt that
provide some default websites and terms you can use should you so wish.
They will need to be moved into the same directory as the main program for use.
