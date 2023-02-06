#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define LIST_LEN 14855
#define WORD_LEN 5
#define INPUT_LEN 64

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREY    "\x1b[37m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[93m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/*
    structure to encapsulate wordle word 

    char[5] name    -   string value of worlde word
                        these values get read in from .csv file
                        and are therefore always lowercase
                        (ex. "adieu", "stomp")

    int possible    -   boolean value represented as integer
                        used to denote wether word is a valid possibility
                        initialized to 1 when the .csv file is read into word_list array
                       ( 0 -> false, 1 -> true)

*/
struct word {
    char name[5];
    int possible;
};


// define array of words, which will contain all five letter words read from 'wordlist.csv'
struct word word_list[LIST_LEN];
const int STRUCT_LEN = sizeof(word_list)/sizeof(word_list[0]);


// prints all words in struct that are still possible
void print_possible_words(){
    printf("All remaining possible words:\n\t"ANSI_COLOR_GREEN);

    // find the possible word with the highest index
    int max = 0;
    for(int i = 0; i < STRUCT_LEN; i++){
        if(word_list[i].possible)
            max = i;
    }

    // Error Check: if no possible words have been found exit
    if(max == 0){
        fprintf(stderr, ANSI_COLOR_RED "Error: all possibilities have been eliminated\n\tRestart the program and double check your inputs don't conflict with one another\n");
        exit(-1);
    }

    // print all possible words, on the last word print a new line
    for(int i = 0; i <= max; i++){
        if(word_list[i].possible)
            i < max ? printf("%s, ", word_list[i].name) : printf("%s", word_list[i].name);
    }
    printf("\n"ANSI_COLOR_RESET);

}


/*

    FILTER FUNCTIONS
        three separate functions for filtering the word_list
        all with identical parameter and return values

        params
            char c  -   character that the function will filter
            int index   -   index where char c appears
                            (only relevant in _yellow and _green function)
        
        returns
            void

        despcription
            filter_words_grey:
                    filters any word that contains the given character
            filter_words_yellow:
                    filters any word that does not contain the given character 
                    or if it only contains the character at the given index
            filter_words_green:
                    filters any word that does not contain the given character
                    at the given index

*/
void filter_words_grey(char c, int index){
    // iterate through struct of words
    for(int i = 0; i < STRUCT_LEN; i++){
        char* word = word_list[i].name;

        // iterate through each word
        if(word_list[i].possible == 1){
            for(int j = 0; j < WORD_LEN; j++){

                // if the character matches, set the possibility of the word to false
                if(word[j] == c){
                    word_list[i].possible = 0;
                    break;
                }

            }
        }

    }
}


void filter_words_yellow(char c, int index){
    // iterate through struct of words
    for(int i = 0; i < STRUCT_LEN; i++){
        char* word = word_list[i].name;

        if(word_list[i].possible == 1){
            int count = 0;

            // count the amount of times char c appears in the current word
            for(int j = 0; j < WORD_LEN; j++){
                if(word[j] == c){
                    count++;
                }
            }

            // if the character only appears at the given index, or not at all; set the possibility of the word to false
            if((count == 0) || (count == 1 && word[index] == c)){
                word_list[i].possible = 0;
            }
        }

    }
}


void filter_words_green(char c, int index){
    // iterate through struct of words
    for(int i = 0; i < STRUCT_LEN; i++){
        char* word = word_list[i].name;

        // if the character is not at the given index, set the possibility of the word to false
        if(word[index] != c){
            word_list[i].possible = 0;
        }
    }
}

// frequency table of english letters according to : https://en.wikipedia.org/wiki/Letter_frequency
double frequency[26] = {
    7.8,    // a 
    2.0,
    4.0,
    3.8,
    11.0,
    1.4,
    3.0,
    2.3,
    8.6,
    0.21,
    0.97,
    5.3,
    2.7,
    7.2,
    6.1,
    2.8,
    0.19,
    7.3,
    8.7,
    6.7,
    3.3,
    1.0,
    0.91,
    0.27,
    1.60,
    0.44    // z
};

char* suggest_next_word(){
    double highestFrequency = 0;
    char* suggested_word;
    int repeat_chars = 0;
    for(int i = 0; i < STRUCT_LEN; i++){
        if(word_list[i].possible){
            double freq = 0;
            for(int j = 0; j < WORD_LEN; j++){
                for(int k = 0; k < WORD_LEN; k++){
                    if(word_list[i].name[k] == word_list[i].name[j] && j != k){
                        repeat_chars = 1;
                    }
                }
                
                if(!repeat_chars)
                    freq += frequency[word_list[i].name[j] - 97];
            }

            if(highestFrequency < freq){
                highestFrequency = freq;
                suggested_word = word_list[i].name;
            }
        }
        repeat_chars = 0;
    }

    return suggested_word;
}

int parse_input(char* str, void (*func) (char, int));


int main(int argc, char* argv[]) {

    // Error Check: one argument must be given via command line
    if(argc != 1){
        fprintf(stderr, ANSI_COLOR_RED "Usage: ./cwordlefinder\n");
        return 1;
    }

    FILE *wordlist = fopen("./wordlist.csv", "r");

    // Error Check: make sure .csv file is found
    if(!wordlist){
        fprintf(stderr, "Can't open wordlist.csv :(\n Make sure the file is in the current directory\n");
        return -1;
    }

    // read from .csv file into array of word structs
    for(int i = 0; i < STRUCT_LEN; i++){
        // initialize struct elements
        int getWord = fscanf(wordlist, "%5s", word_list[i].name);
        word_list[i].possible = 1;

        // Error Check: make sure you read one line from .csv at a time
        if(getWord != 1){
            perror("Error: Parsing .csv file\n");
            return -1;
        }
    }

    fclose(wordlist);
    printf(ANSI_COLOR_GREEN "WORDLESOLVER"  ANSI_COLOR_RESET
    "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf(
    "\tEnter answers in letter index format\t(ex. 'a1 d2 e5')\n"
    "\tIf there are no new letters for the given entry just hit enter\n"
    "\tIt is suggested your first guess should be " ANSI_COLOR_CYAN"%s\n" ANSI_COLOR_RESET
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n", suggest_next_word());

    int solved = 0;

    // query user for new grey, yello, and green characters until they specify that the solution was found
    while(!solved){

        int status_code = 1;
 
        while(status_code != 0){
            char grey_str[INPUT_LEN] = {0};
            // take and parse input for new grey letters
            printf("Enter " ANSI_COLOR_GREY "grey" ANSI_COLOR_RESET " letters: "  ANSI_COLOR_GREY);
            fgets(grey_str, INPUT_LEN, stdin);
            status_code = parse_input(grey_str, &filter_words_grey);
        }
        printf(ANSI_COLOR_RESET);

        status_code = 1;
        while (status_code != 0)
        {
            char yellow_str[INPUT_LEN] = {0};
            // take and parse input for new yellow letters
            printf("Enter " ANSI_COLOR_YELLOW "yellow" ANSI_COLOR_RESET " letters: " ANSI_COLOR_YELLOW);
            fgets(yellow_str, INPUT_LEN, stdin);
            status_code = parse_input(yellow_str, &filter_words_yellow);
        }
        printf(ANSI_COLOR_RESET);
        
        status_code = 1;
        while(status_code != 0){
            char green_str[INPUT_LEN] = {0};
            // take and parse input for new green letters
            printf("Enter " ANSI_COLOR_GREEN "green " ANSI_COLOR_RESET "letters: " ANSI_COLOR_GREEN);
            fgets(green_str, INPUT_LEN, stdin);
            status_code = parse_input(green_str, &filter_words_green);
        }
        printf(ANSI_COLOR_RESET);


        // print all words that are still possible
        print_possible_words();
        printf(ANSI_COLOR_CYAN "Suggested Next Word: %s\n" ANSI_COLOR_RESET, suggest_next_word());

        // ask if the user has found the correct word
        while(1){
            char yn_str[INPUT_LEN] = {0};
            printf("\n\nHas the wordle been solved?[y/n]: ");
            fgets(yn_str, INPUT_LEN, stdin);
            // only accept responses 'y\n', 'Y\n', 'n\n', and 'N\n' as valid
            if((yn_str[0] == 'y' || yn_str[0] == 'Y') && yn_str[1] == '\n'){
                solved = 1;
                break;
            }
            if((yn_str[0] == 'n' || yn_str[0] == 'N') && yn_str[1] == '\n'){
                break;
            }
            // the program will loop forever until a valid input is given
        }
        printf("\n\n");
    }

    printf(ANSI_COLOR_CYAN"Goodbye 👋\n");
    return 0;

}


/*
    parse_input
        params
                char* str   -   input string
                                ex. 'd2 i3 u5\n\0'  

                void (*func) (char, int)    -   function pointer
                                                used to call filter function to eliminate words 
                                                pass in the address of the corresponding function
                                                    ex. &filter_words_green
        returns
                int     -   status of function
                             0: Success
                            -1: Error

        description
                tokenizes input string
                pulls out each letter and index and 
                filters the word list depending on the
                filter function passed in as the second argument
*/
int parse_input(char* str, void (*func) (char, int)){
    // tokenize input string (peel off a single term ie. 'a1')
    char *token;
    token = strtok(str, " ");


    // if the string is empty there is nothing to do
    if(strcmp(str, "\n") != 0 || str[0] == '\n'){
        
        // continue to tokenize until you hit a new line
        while(token != NULL && strcmp(token, "\n") != 0){

            // Error Check: length of token should be two characters long ie. 'a4', except when its a new line then it can be three ie. 'a4\n'
            if((strlen(token) != 2 && token[2] != '\n') || token[1] == '\n'){
                printf(ANSI_COLOR_RED "Error: answer should be in letter index format (ex. 'a1 d2 e5')\n" ANSI_COLOR_RESET);
                return -1;
            }

            // get the letter and index of current token
            char letter = tolower(token[0]);
            int index = atoi(&token[1]) -1;

            // Error Check: index of the current token must be between 0-4
            if(index > 4 || index < 0){
                fprintf(stderr, ANSI_COLOR_RED 
                        "Error: invalid index (%c%d can't exist on the board)\n\tTry entering the string in the following format: 'a1 d2 e5'\n" ANSI_COLOR_RESET,
                        letter, index+1);
                return -1;
            }

            // call function pointer to filter remaining word list
            (*func)(letter, index);

            // get the next token
            token = strtok(NULL, " "); //first argument NULL lets function know to to continue tokenizing the initial string
        }

        return 0;
    }
    
    return -1;
}