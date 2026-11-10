/**
 * library_core.h - Streamlined Library Management System Core
 * Clean, lightweight C data structures and function prototypes.
 */

#ifndef LIBRARY_CORE_H
#define LIBRARY_CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(d) _mkdir(d)
#else
    #include <sys/stat.h>
    #define MKDIR(d) mkdir(d, 0777)
#endif

#define TEXT_SIZE 100
#define DESC_SIZE 256

/* Data structures */
typedef struct {
    int id;
    char title[TEXT_SIZE];
    char author[TEXT_SIZE];
    char category[TEXT_SIZE];
    int total_copies;
    int available_copies;
} Book;

typedef struct {
    int id;
    char name[TEXT_SIZE];
    char email[TEXT_SIZE];
    char phone[TEXT_SIZE];
    char department[TEXT_SIZE];
    int active; /* 1 = active, 0 = deleted */
} Member;

typedef struct {
    int id;
    int book_id;
    int member_id;
    int returned; /* 0 = active loan, 1 = returned */
} Loan;

/* Storage paths */
void init_paths(void);
void ensure_data_dir(void);
void seed_sample_data_if_empty(void);

/* Book operations */
int find_book(FILE *file, int id, Book *result, long *position);
int get_book_by_id(int id, Book *book);
int add_book_record(const Book *b, char *err, size_t err_len);
int edit_book_record(const Book *b, char *err, size_t err_len);
int delete_book_record(int id, char *err, size_t err_len);

/* Member operations */
int find_member(FILE *file, int id, Member *result, long *position);
int get_member_by_id(int id, Member *member);
int get_member_loan_count(int member_id);
int add_member_record(const Member *m, char *err, size_t err_len);
int edit_member_record(const Member *m, char *err, size_t err_len);
int delete_member_record(int id, char *err, size_t err_len);

/* Loan operations */
int issue_book_record(int book_id, int member_id, char *err, size_t err_len);
int return_book_record(int loan_id, char *err, size_t err_len);

/* Interactive CLI */
void interactive_menu(void);

/* API Handlers */
int handle_api_action(const char *action, const char *payload);

#endif /* LIBRARY_CORE_H */
