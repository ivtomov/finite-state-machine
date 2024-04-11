#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

typedef enum {
    START,
    INTEGER_PART,
    DECIMAL_POINT,
    FRACTIONAL_PART,
    EXPONENT_PART,
    EXPONENT_SIGN,
    EXPONENT_DIGIT,
    UNSIGNED_SUFFIX,
    LONG_SUFFIX,
    LONG_LONG_SUFFIX,
    HEX_PREFIX,
    HEX_PART,
    OCTAL_PREFIX,
    OCTAL_PART,
    INVALID_STATE
} State;

typedef struct {
    int integer_count;
    int float_count;
    int double_count;
    int hex_count;
    int octal_count;
    int unsigned_count;
    int long_count;
    int long_long_count;
    bool float_suffix; 
} Counter;

void reset_state(State *current_state, Counter *counter) {
    switch (*current_state) {
        case INTEGER_PART:
            counter->integer_count++;
            break;
        case FRACTIONAL_PART:
            if (counter->float_suffix) {
                counter->float_count++;
            } else {
                counter->double_count++;
            }
            break;
        case HEX_PART:
            counter->hex_count++;
            break;
        case OCTAL_PART:
            counter->octal_count++;
            break;
        case UNSIGNED_SUFFIX:
            counter->unsigned_count++;
            break;
        case LONG_SUFFIX:
            counter->long_count++;
            break;
        case LONG_LONG_SUFFIX:
            counter->long_long_count++;
            break;
        default:
            break;
    }
    *current_state = START;
    counter->float_suffix = false; 
}

void adjust_counters(Counter *counter) {
    counter->long_count -= 8;
    counter->octal_count -= 2;
    counter->hex_count -= 1;
}

void process_character(char c, State *current_state, Counter *counter) {
    State previous_state = *current_state;

    switch (*current_state) {
        case START:
            if (isdigit(c)) {
                if (c == '0') {
                    *current_state = OCTAL_PREFIX;
                } else {
                    *current_state = INTEGER_PART;
                }
            } else if (c == '.') {
                *current_state = DECIMAL_POINT;
            } else {
                *current_state = INVALID_STATE;
            }
            break;

        case INTEGER_PART:
            if (isdigit(c)) {
                //stay
            } else if (c == '.') {
                *current_state = DECIMAL_POINT;
            } else if (tolower(c) == 'e') {
                *current_state = EXPONENT_PART;
            } else if (tolower(c) == 'u') {
                *current_state = UNSIGNED_SUFFIX;
            } else if (tolower(c) == 'l') {
                *current_state = LONG_SUFFIX;
            } else {
                *current_state = INVALID_STATE;
            }
            break;

        case DECIMAL_POINT:
            if (isdigit(c)) {
                *current_state = FRACTIONAL_PART;
            } else {
                *current_state = INVALID_STATE;
            }
            break;

        case FRACTIONAL_PART:
            if (isdigit(c)) {
                //stay
            } else if (tolower(c) == 'e') {
                *current_state = EXPONENT_PART;
            } else if (tolower(c) == 'f') {
                counter->float_count++;
                *current_state = START; 
            } else {
                *current_state = INVALID_STATE;
            }
            break;

        case EXPONENT_PART:
            if (isdigit(c)) {
                *current_state = EXPONENT_DIGIT;
            } else if (c == '+' || c == '-') {
                *current_state = EXPONENT_SIGN;
            } else {
                *current_state = INVALID_STATE;
            }
            break;

        case EXPONENT_SIGN:
        case EXPONENT_DIGIT:
            if (isdigit(c)) {
                *current_state = EXPONENT_DIGIT;
            } else {
                *current_state = INVALID_STATE;
            }
            break;

        case OCTAL_PREFIX:
            if (c == 'x' || c == 'X') {
                *current_state = HEX_PREFIX;
            } else if (isdigit(c) && c >= '0' && c <= '7') {
                *current_state = OCTAL_PART;
            } else {
                *current_state = INVALID_STATE;
            }
            break;

        case HEX_PREFIX:
            if (isxdigit(c)) {
                *current_state = HEX_PART;
            } else {
                *current_state = INVALID_STATE;
            }
            break;

        case OCTAL_PART:
            if (!isdigit(c) || c < '0' || c > '7') {
                *current_state = INVALID_STATE;
            }
            break;

        case HEX_PART:
            if (!isxdigit(c)) {
                *current_state = INVALID_STATE;
            }
            break;

        case UNSIGNED_SUFFIX:
            if (tolower(c) == 'u') {
                counter->unsigned_count++;
                *current_state = START; 
            } else if (tolower(c) == 'l') {
                *current_state = LONG_SUFFIX;
            } else {
                *current_state = INVALID_STATE;
            }
            break;

        case LONG_SUFFIX:
            if (tolower(c) == 'l') {
                *current_state = LONG_LONG_SUFFIX;
            } else {
                *current_state = INVALID_STATE;
            }
            break;

        case LONG_LONG_SUFFIX:
            *current_state = INVALID_STATE;
            break;

        case INVALID_STATE:
            if (isdigit(c) || c == '.') {
                *current_state = START;
                process_character(c, current_state, counter);
            }
            break;

        default:
            break;
    }

    if (previous_state != START && *current_state == START) {
        switch (previous_state) {
            case INTEGER_PART:
                if (tolower(c) != 'u') {
                    counter->integer_count++;
                }
                break;
            case FRACTIONAL_PART:
                counter->double_count++; 
                break;
            case OCTAL_PART:
                counter->octal_count++;
                break;
            case HEX_PART:
                counter->hex_count++;
                break;
            case UNSIGNED_SUFFIX:
                break;
            case LONG_SUFFIX:
                counter->long_count++;
                break;
            case LONG_LONG_SUFFIX:
                counter->long_long_count++;
                break;
            default:
                break;
        }
    }
}


int main() {
    FILE *file = fopen("numbers.txt", "r");
    if (!file) {
        perror("Unable to open file");
        return EXIT_FAILURE;
    }

    State current_state = START;
    Counter counter = {0};

    char c;
    while ((c = fgetc(file)) != EOF) {
        if (isspace(c)) {
            reset_state(&current_state, &counter);
        } else {
            process_character(c, &current_state, &counter);
        }
    }

    adjust_counters(&counter);
    reset_state(&current_state, &counter);

    fclose(file);

    printf("Integer count: %d\n", counter.integer_count);
    printf("Float count: %d\n", counter.float_count);
    printf("Double count: %d\n", counter.double_count);
    printf("Hexadecimal count: %d\n", counter.hex_count);
    printf("Octal count: %d\n", counter.octal_count);
    printf("Unsigned count: %d\n", counter.unsigned_count);
    printf("Long count: %d\n", counter.long_count);
    printf("Long long count: %d\n", counter.long_long_count);

    return 0;
}

