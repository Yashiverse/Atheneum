#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE "books.dat"
#define TEXT_SIZE 100

typedef struct {
    int id;
    char title[TEXT_SIZE];
    char author[TEXT_SIZE];
    int total_copies;
    int available_copies;
} Book;

/* Read a line of input and remove its trailing newline. */
static int read_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    } else {
        /* Discard the rest of an overlong input line. */
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }

    return 1;
}

/* Read an integer, repeating the prompt if the input is invalid. */
static int read_int(const char *prompt, int *value) {
    char input[100];

    for (;;) {
        if (!read_line(prompt, input, sizeof(input))) {
            return 0;
        }

        char *end;
        long number = strtol(input, &end, 10);

        while (isspace((unsigned char)*end)) {
            end++;
        }

        if (input[0] != '\0' && *end == '\0') {
            *value = (int)number;
            return 1;
        }

        printf("Please enter a valid whole number.\n");
    }
}

/* Find a book by ID. If found, return its record and file position. */
static int find_book(FILE *file, int id, Book *result, long *position) {
    Book book;
    rewind(file);

    while (fread(&book, sizeof(Book), 1, file) == 1) {
        long record_position = ftell(file) - (long)sizeof(Book);

        if (book.id == id) {
            if (result != NULL) {
                *result = book;
            }
            if (position != NULL) {
                *position = record_position;
            }
            return 1;
        }
    }

    return 0;
}

static void add_book(void) {
    Book book = {0};
    FILE *file = fopen(DATA_FILE, "ab+");

    if (file == NULL) {
        perror("Could not open the data file");
        return;
    }

    if (!read_int("Book ID: ", &book.id) || book.id <= 0) {
        printf("Book ID must be a positive number.\n");
        fclose(file);
        return;
    }

    if (find_book(file, book.id, NULL, NULL)) {
        printf("A book with that ID already exists.\n");
        fclose(file);
        return;
    }

    if (!read_line("Title: ", book.title, sizeof(book.title)) ||
        !read_line("Author: ", book.author, sizeof(book.author))) {
        fclose(file);
        return;
    }

    if (book.title[0] == '\0' || book.author[0] == '\0') {
        printf("Title and author cannot be empty.\n");
        fclose(file);
        return;
    }

    if (!read_int("Number of copies: ", &book.total_copies) ||
        book.total_copies <= 0) {
        printf("Number of copies must be positive.\n");
        fclose(file);
        return;
    }

    book.available_copies = book.total_copies;

    if (fseek(file, 0, SEEK_END) != 0 ||
        fwrite(&book, sizeof(Book), 1, file) != 1) {
        perror("Could not save the book");
        fclose(file);
        return;
    }

    printf("Book added successfully.\n");
    fclose(file);
}

static void list_books(void) {
    FILE *file = fopen(DATA_FILE, "rb");

    if (file == NULL) {
        printf("No books found yet. Add a book first.\n");
        return;
    }

    Book book;
    int found = 0;

    printf("\n%-8s %-30s %-25s %-10s %-10s\n",
           "ID", "Title", "Author", "Total", "Available");
    printf("----------------------------------------------------------------------------------------\n");

    while (fread(&book, sizeof(Book), 1, file) == 1) {
        printf("%-8d %-30s %-25s %-10d %-10d\n",
               book.id, book.title, book.author,
               book.total_copies, book.available_copies);
        found = 1;
    }

    if (!found) {
        printf("No books found.\n");
    }

    fclose(file);
}

static void search_books(void) {
    char query[TEXT_SIZE];

    if (!read_line("Enter title to search for: ", query, sizeof(query))) {
        return;
    }

    FILE *file = fopen(DATA_FILE, "rb");
    if (file == NULL) {
        printf("No books found yet. Add a book first.\n");
        return;
    }

    Book book;
    int found = 0;

    while (fread(&book, sizeof(Book), 1, file) == 1) {
        /* Title search is case-sensitive. */
        if (strstr(book.title, query) != NULL) {
            printf("\nID: %d\nTitle: %s\nAuthor: %s\n"
                   "Total copies: %d\nAvailable copies: %d\n",
                   book.id, book.title, book.author,
                   book.total_copies, book.available_copies);
            found = 1;
        }
    }

    if (!found) {
        printf("No matching books found.\n");
    }

    fclose(file);
}

static void update_copies(int issuing) {
    int id;
    Book book;
    long position;

    if (!read_int("Book ID: ", &id)) {
        return;
    }

    FILE *file = fopen(DATA_FILE, "r+b");
    if (file == NULL) {
        printf("No books found yet. Add a book first.\n");
        return;
    }

    if (!find_book(file, id, &book, &position)) {
        printf("Book ID not found.\n");
        fclose(file);
        return;
    }

    if (issuing) {
        if (book.available_copies == 0) {
            printf("No copies are currently available.\n");
            fclose(file);
            return;
        }
        book.available_copies--;
        printf("Book issued successfully.\n");
    } else {
        if (book.available_copies >= book.total_copies) {
            printf("All copies are already in the library.\n");
            fclose(file);
            return;
        }
        book.available_copies++;
        printf("Book returned successfully.\n");
    }

    if (fseek(file, position, SEEK_SET) != 0 ||
        fwrite(&book, sizeof(Book), 1, file) != 1) {
        perror("Could not update the book record");
    }

    fclose(file);
}

int main(void) {
    int choice;

    for (;;) {
        printf("\n=== Library Management System ===\n");
        printf("1. Add a book\n");
        printf("2. List all books\n");
        printf("3. Search books by title\n");
        printf("4. Issue a book\n");
        printf("5. Return a book\n");
        printf("0. Exit\n");

        if (!read_int("Choose an option: ", &choice)) {
            printf("\nGoodbye.\n");
            break;
        }

        switch (choice) {
            case 1:
                add_book();
                break;
            case 2:
                list_books();
                break;
            case 3:
                search_books();
                break;
            case 4:
                update_copies(1);
                break;
            case 5:
                update_copies(0);
                break;
            case 0:
                printf("Goodbye.\n");
                return 0;
            default:
                printf("Invalid option. Please try again.\n");
        }
    }

    return 0;
}