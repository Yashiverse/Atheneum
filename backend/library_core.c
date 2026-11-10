/**
 * library_core.c - Streamlined C Core for Library Management System
 * Preserves original C data structures and binary file routines.
 * Supports both Interactive Console CLI and clean JSON API mode.
 */

#include "library_core.h"

/* File paths */
static char g_data_dir[512] = "data";
static char g_books_file[512] = "data/books.dat";
static char g_members_file[512] = "data/members.dat";
static char g_loans_file[512] = "data/loans.dat";

void init_paths(void) {
    FILE *f = fopen("data/books.dat", "rb");
    if (f) {
        fclose(f);
        return;
    }
    FILE *fb = fopen("backend/data/books.dat", "rb");
    if (fb) {
        fclose(fb);
        snprintf(g_data_dir, sizeof(g_data_dir), "backend/data");
        snprintf(g_books_file, sizeof(g_books_file), "backend/data/books.dat");
        snprintf(g_members_file, sizeof(g_members_file), "backend/data/members.dat");
        snprintf(g_loans_file, sizeof(g_loans_file), "backend/data/loans.dat");
        return;
    }
    MKDIR("backend/data");
    snprintf(g_data_dir, sizeof(g_data_dir), "backend/data");
    snprintf(g_books_file, sizeof(g_books_file), "backend/data/books.dat");
    snprintf(g_members_file, sizeof(g_members_file), "backend/data/members.dat");
    snprintf(g_loans_file, sizeof(g_loans_file), "backend/data/loans.dat");
}

void ensure_data_dir(void) {
    MKDIR(g_data_dir);
}

/* Helper to safely copy strings without buffer overrun */
static void safe_copy(char *dest, const char *src, size_t dest_size) {
    if (!dest || dest_size == 0) return;
    if (!src) { dest[0] = '\0'; return; }
    snprintf(dest, dest_size, "%s", src);
}

/* ========================================================================= */
/* BOOK OPERATIONS                                                           */
/* ========================================================================= */

int find_book(FILE *file, int id, Book *result, long *position) {
    if (!file) return 0;
    Book book;
    rewind(file);

    while (fread(&book, sizeof(Book), 1, file) == 1) {
        long record_pos = ftell(file) - (long)sizeof(Book);
        if (book.id == id) {
            if (result) *result = book;
            if (position) *position = record_pos;
            return 1;
        }
    }
    return 0;
}

int get_book_by_id(int id, Book *book) {
    ensure_data_dir();
    FILE *f = fopen(g_books_file, "rb");
    if (!f) return 0;
    int found = find_book(f, id, book, NULL);
    fclose(f);
    return found;
}

int add_book_record(const Book *b, char *err, size_t err_len) {
    ensure_data_dir();
    if (b->id <= 0) {
        snprintf(err, err_len, "Book ID must be a positive number.");
        return 0;
    }
    if (strlen(b->title) == 0 || strlen(b->author) == 0) {
        snprintf(err, err_len, "Title and author cannot be empty.");
        return 0;
    }
    if (b->total_copies <= 0) {
        snprintf(err, err_len, "Number of copies must be positive.");
        return 0;
    }

    FILE *file = fopen(g_books_file, "ab+");
    if (!file) {
        snprintf(err, err_len, "Could not open books data file.");
        return 0;
    }

    if (find_book(file, b->id, NULL, NULL)) {
        snprintf(err, err_len, "A book with ID %d already exists.", b->id);
        fclose(file);
        return 0;
    }

    Book book = *b;
    if (strlen(book.category) == 0) {
        safe_copy(book.category, "General", sizeof(book.category));
    }
    book.available_copies = book.total_copies;

    if (fseek(file, 0, SEEK_END) != 0 || fwrite(&book, sizeof(Book), 1, file) != 1) {
        snprintf(err, err_len, "Failed to write book record to file.");
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

int edit_book_record(const Book *updated, char *err, size_t err_len) {
    ensure_data_dir();
    FILE *file = fopen(g_books_file, "r+b");
    if (!file) {
        snprintf(err, err_len, "Books file not found.");
        return 0;
    }

    Book existing;
    long pos;
    if (!find_book(file, updated->id, &existing, &pos)) {
        snprintf(err, err_len, "Book ID %d not found.", updated->id);
        fclose(file);
        return 0;
    }

    if (strlen(updated->title) == 0 || strlen(updated->author) == 0) {
        snprintf(err, err_len, "Title and author cannot be empty.");
        fclose(file);
        return 0;
    }
    if (updated->total_copies <= 0) {
        snprintf(err, err_len, "Total copies must be positive.");
        fclose(file);
        return 0;
    }

    int borrowed = existing.total_copies - existing.available_copies;
    if (updated->total_copies < borrowed) {
        snprintf(err, err_len, "Total copies (%d) cannot be less than currently issued copies (%d).",
                 updated->total_copies, borrowed);
        fclose(file);
        return 0;
    }

    Book book = existing;
    safe_copy(book.title, updated->title, sizeof(book.title));
    safe_copy(book.author, updated->author, sizeof(book.author));
    if (strlen(updated->category) > 0) {
        safe_copy(book.category, updated->category, sizeof(book.category));
    }
    book.total_copies = updated->total_copies;
    book.available_copies = updated->total_copies - borrowed;

    if (fseek(file, pos, SEEK_SET) != 0 || fwrite(&book, sizeof(Book), 1, file) != 1) {
        snprintf(err, err_len, "Failed to update book on disk.");
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

int delete_book_record(int id, char *err, size_t err_len) {
    ensure_data_dir();
    FILE *file = fopen(g_books_file, "rb");
    if (!file) {
        snprintf(err, err_len, "Books file not found.");
        return 0;
    }

    Book target;
    if (!find_book(file, id, &target, NULL)) {
        snprintf(err, err_len, "Book ID %d not found.", id);
        fclose(file);
        return 0;
    }

    if (target.available_copies < target.total_copies) {
        snprintf(err, err_len, "Cannot delete book while %d copies are issued out.",
                 target.total_copies - target.available_copies);
        fclose(file);
        return 0;
    }

    char tmp_path[1024];
    snprintf(tmp_path, sizeof(tmp_path), "%s/books_tmp.dat", g_data_dir);
    FILE *temp = fopen(tmp_path, "wb");
    if (!temp) {
        snprintf(err, err_len, "Could not create temporary file for deletion.");
        fclose(file);
        return 0;
    }

    Book cur;
    rewind(file);
    while (fread(&cur, sizeof(Book), 1, file) == 1) {
        if (cur.id != id) {
            fwrite(&cur, sizeof(Book), 1, temp);
        }
    }

    fclose(file);
    fclose(temp);
    remove(g_books_file);
    rename(tmp_path, g_books_file);
    return 1;
}

/* ========================================================================= */
/* MEMBER OPERATIONS                                                         */
/* ========================================================================= */

int find_member(FILE *file, int id, Member *result, long *position) {
    if (!file) return 0;
    Member member;
    rewind(file);

    while (fread(&member, sizeof(Member), 1, file) == 1) {
        long record_pos = ftell(file) - (long)sizeof(Member);
        if (member.id == id) {
            if (result) *result = member;
            if (position) *position = record_pos;
            return 1;
        }
    }
    return 0;
}

int get_member_by_id(int id, Member *member) {
    ensure_data_dir();
    FILE *f = fopen(g_members_file, "rb");
    if (!f) return 0;
    int found = find_member(f, id, member, NULL);
    fclose(f);
    return found;
}

int get_member_loan_count(int member_id) {
    ensure_data_dir();
    FILE *f = fopen(g_loans_file, "rb");
    if (!f) return 0;
    Loan loan;
    int count = 0;
    while (fread(&loan, sizeof(Loan), 1, f) == 1) {
        if (loan.member_id == member_id && loan.returned == 0) {
            count++;
        }
    }
    fclose(f);
    return count;
}

int add_member_record(const Member *m, char *err, size_t err_len) {
    ensure_data_dir();
    if (m->id <= 0) {
        snprintf(err, err_len, "Member ID must be positive.");
        return 0;
    }
    if (strlen(m->name) == 0 || strlen(m->email) == 0) {
        snprintf(err, err_len, "Member name and email are required.");
        return 0;
    }

    FILE *file = fopen(g_members_file, "ab+");
    if (!file) {
        snprintf(err, err_len, "Could not open members database.");
        return 0;
    }

    if (find_member(file, m->id, NULL, NULL)) {
        snprintf(err, err_len, "Member ID %d already exists.", m->id);
        fclose(file);
        return 0;
    }

    Member member = *m;
    member.active = 1;

    if (fseek(file, 0, SEEK_END) != 0 || fwrite(&member, sizeof(Member), 1, file) != 1) {
        snprintf(err, err_len, "Failed to save member record.");
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

int edit_member_record(const Member *updated, char *err, size_t err_len) {
    ensure_data_dir();
    FILE *file = fopen(g_members_file, "r+b");
    if (!file) {
        snprintf(err, err_len, "Members database not found.");
        return 0;
    }

    Member existing;
    long pos;
    if (!find_member(file, updated->id, &existing, &pos)) {
        snprintf(err, err_len, "Member ID %d not found.", updated->id);
        fclose(file);
        return 0;
    }

    if (strlen(updated->name) == 0 || strlen(updated->email) == 0) {
        snprintf(err, err_len, "Name and email cannot be empty.");
        fclose(file);
        return 0;
    }

    Member m = existing;
    safe_copy(m.name, updated->name, sizeof(m.name));
    safe_copy(m.email, updated->email, sizeof(m.email));
    safe_copy(m.phone, updated->phone, sizeof(m.phone));
    safe_copy(m.department, updated->department, sizeof(m.department));

    if (fseek(file, pos, SEEK_SET) != 0 || fwrite(&m, sizeof(Member), 1, file) != 1) {
        snprintf(err, err_len, "Failed to update member record.");
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

int delete_member_record(int id, char *err, size_t err_len) {
    ensure_data_dir();
    int active_loans = get_member_loan_count(id);
    if (active_loans > 0) {
        snprintf(err, err_len, "Cannot delete member with %d unreturned book(s).", active_loans);
        return 0;
    }

    FILE *file = fopen(g_members_file, "rb");
    if (!file) {
        snprintf(err, err_len, "Members file not found.");
        return 0;
    }

    Member target;
    if (!find_member(file, id, &target, NULL)) {
        snprintf(err, err_len, "Member ID %d not found.", id);
        fclose(file);
        return 0;
    }

    char tmp_path[1024];
    snprintf(tmp_path, sizeof(tmp_path), "%s/members_tmp.dat", g_data_dir);
    FILE *temp = fopen(tmp_path, "wb");
    if (!temp) {
        snprintf(err, err_len, "Could not create temporary file for deletion.");
        fclose(file);
        return 0;
    }

    Member cur;
    rewind(file);
    while (fread(&cur, sizeof(Member), 1, file) == 1) {
        if (cur.id != id) {
            fwrite(&cur, sizeof(Member), 1, temp);
        }
    }

    fclose(file);
    fclose(temp);
    remove(g_members_file);
    rename(tmp_path, g_members_file);
    return 1;
}

/* ========================================================================= */
/* ISSUE & RETURN OPERATIONS                                                 */
/* ========================================================================= */

int issue_book_record(int book_id, int member_id, char *err, size_t err_len) {
    ensure_data_dir();

    /* 1. Validate Book */
    FILE *bfile = fopen(g_books_file, "r+b");
    if (!bfile) {
        snprintf(err, err_len, "Books file not found.");
        return 0;
    }

    Book book;
    long bpos;
    if (!find_book(bfile, book_id, &book, &bpos)) {
        snprintf(err, err_len, "Book ID %d not found.", book_id);
        fclose(bfile);
        return 0;
    }

    if (book.available_copies <= 0) {
        snprintf(err, err_len, "No copies of '%s' are currently available.", book.title);
        fclose(bfile);
        return 0;
    }

    /* 2. Validate Member */
    Member member;
    if (!get_member_by_id(member_id, &member) || !member.active) {
        snprintf(err, err_len, "Member ID %d not found.", member_id);
        fclose(bfile);
        return 0;
    }

    /* 3. Decrement available copies (Original C Logic) */
    book.available_copies--;
    if (fseek(bfile, bpos, SEEK_SET) != 0 || fwrite(&book, sizeof(Book), 1, bfile) != 1) {
        snprintf(err, err_len, "Failed to update book copies on disk.");
        fclose(bfile);
        return 0;
    }
    fclose(bfile);

    /* 4. Append Loan Record */
    FILE *lfile = fopen(g_loans_file, "ab+");
    if (!lfile) {
        snprintf(err, err_len, "Could not open loans file.");
        return 0;
    }

    fseek(lfile, 0, SEEK_END);
    long size = ftell(lfile);
    int next_id = (int)(size / sizeof(Loan)) + 1;

    Loan loan;
    loan.id = next_id;
    loan.book_id = book_id;
    loan.member_id = member_id;
    loan.returned = 0;

    fwrite(&loan, sizeof(Loan), 1, lfile);
    fclose(lfile);
    return 1;
}

int return_book_record(int loan_id, char *err, size_t err_len) {
    ensure_data_dir();

    FILE *lfile = fopen(g_loans_file, "r+b");
    if (!lfile) {
        snprintf(err, err_len, "Loans file not found.");
        return 0;
    }

    Loan loan;
    long lpos = -1;
    int found_loan = 0;
    rewind(lfile);

    while (fread(&loan, sizeof(Loan), 1, lfile) == 1) {
        if (loan.id == loan_id) {
            lpos = ftell(lfile) - (long)sizeof(Loan);
            found_loan = 1;
            break;
        }
    }

    if (!found_loan) {
        snprintf(err, err_len, "Loan record #%d not found.", loan_id);
        fclose(lfile);
        return 0;
    }

    if (loan.returned) {
        snprintf(err, err_len, "Book has already been returned.");
        fclose(lfile);
        return 0;
    }

    /* 2. Increment book copies (Original C Logic) */
    FILE *bfile = fopen(g_books_file, "r+b");
    if (!bfile) {
        snprintf(err, err_len, "Books file not found.");
        fclose(lfile);
        return 0;
    }

    Book book;
    long bpos;
    if (!find_book(bfile, loan.book_id, &book, &bpos)) {
        snprintf(err, err_len, "Book ID %d not found.", loan.book_id);
        fclose(bfile);
        fclose(lfile);
        return 0;
    }

    if (book.available_copies < book.total_copies) {
        book.available_copies++;
    }

    if (fseek(bfile, bpos, SEEK_SET) != 0 || fwrite(&book, sizeof(Book), 1, bfile) != 1) {
        snprintf(err, err_len, "Failed to update book copies on disk.");
        fclose(bfile);
        fclose(lfile);
        return 0;
    }
    fclose(bfile);

    /* 3. Mark loan returned */
    loan.returned = 1;
    if (fseek(lfile, lpos, SEEK_SET) != 0 || fwrite(&loan, sizeof(Loan), 1, lfile) != 1) {
        snprintf(err, err_len, "Failed to update loan record.");
        fclose(lfile);
        return 0;
    }

    fclose(lfile);
    return 1;
}

/* ========================================================================= */
/* SAMPLE DATA SEEDING                                                       */
/* ========================================================================= */

void seed_sample_data_if_empty(void) {
    ensure_data_dir();
    FILE *f = fopen(g_books_file, "rb");
    if (f) {
        fclose(f);
        return;
    }

    Book books[] = {
        {101, "The Great Gatsby", "F. Scott Fitzgerald", "Fiction", 5, 4},
        {102, "To Kill a Mockingbird", "Harper Lee", "Fiction", 4, 3},
        {103, "Clean Code", "Robert C. Martin", "Computer Science", 6, 5},
        {104, "Design Patterns", "Erich Gamma et al.", "Computer Science", 4, 3},
        {105, "Introduction to Algorithms", "Thomas H. Cormen", "Computer Science", 3, 2},
        {106, "1984", "George Orwell", "Dystopian", 5, 5},
        {107, "A Brief History of Time", "Stephen Hawking", "Science", 4, 4},
        {108, "The Pragmatic Programmer", "David Thomas", "Computer Science", 5, 4}
    };

    FILE *bf = fopen(g_books_file, "wb");
    if (bf) {
        for (size_t i = 0; i < sizeof(books)/sizeof(books[0]); i++) {
            fwrite(&books[i], sizeof(Book), 1, bf);
        }
        fclose(bf);
    }

    Member members[] = {
        {1, "Alice Walker", "alice.w@univ.edu", "555-0101", "Computer Science", 1},
        {2, "Marcus Vance", "m.vance@univ.edu", "555-0102", "Literature", 1},
        {3, "Clara Oswald", "c.oswald@univ.edu", "555-0103", "Physics", 1},
        {4, "Julian Reynolds", "j.reynolds@univ.edu", "555-0104", "Engineering", 1}
    };

    FILE *mf = fopen(g_members_file, "wb");
    if (mf) {
        for (size_t i = 0; i < sizeof(members)/sizeof(members[0]); i++) {
            fwrite(&members[i], sizeof(Member), 1, mf);
        }
        fclose(mf);
    }

    Loan loans[] = {
        {1, 101, 1, 0},
        {2, 102, 2, 0},
        {3, 103, 3, 0},
        {4, 104, 4, 0},
        {5, 105, 1, 0},
        {6, 108, 2, 0}
    };

    FILE *lf = fopen(g_loans_file, "wb");
    if (lf) {
        for (size_t i = 0; i < sizeof(loans)/sizeof(loans[0]); i++) {
            fwrite(&loans[i], sizeof(Loan), 1, lf);
        }
        fclose(lf);
    }
}

/* ========================================================================= */
/* JSON HELPERS                                                              */
/* ========================================================================= */

static void json_esc(const char *src, char *dest, size_t max_len) {
    size_t d = 0;
    for (size_t s = 0; src && src[s] != '\0' && d + 2 < max_len; s++) {
        unsigned char c = (unsigned char)src[s];
        if (c == '"') { dest[d++] = '\\'; dest[d++] = '"'; }
        else if (c == '\\') { dest[d++] = '\\'; dest[d++] = '\\'; }
        else if (c == '\n') { dest[d++] = '\\'; dest[d++] = 'n'; }
        else if (c == '\r') { dest[d++] = '\\'; dest[d++] = 'r'; }
        else if (c == '\t') { dest[d++] = '\\'; dest[d++] = 't'; }
        else if (c >= 32) { dest[d++] = (char)c; }
    }
    dest[d] = '\0';
}

static int json_str(const char *json, const char *key, char *out, size_t max_len) {
    if (!json || !key || !out || max_len == 0) return 0;
    out[0] = '\0';
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return 0;
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t' || *p == ':') p++;
    if (*p != '"') return 0;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i + 1 < max_len) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return 1;
}

static int json_int(const char *json, const char *key, int *out) {
    if (!json || !key || !out) return 0;
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return 0;
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t' || *p == ':') p++;
    if (*p == '"') p++;
    char *end;
    long v = strtol(p, &end, 10);
    if (end != p) {
        *out = (int)v;
        return 1;
    }
    return 0;
}

static int match_str(const char *haystack, const char *needle) {
    if (!needle || strlen(needle) == 0) return 1;
    if (!haystack) return 0;
    size_t hlen = strlen(haystack), nlen = strlen(needle);
    if (nlen > hlen) return 0;
    for (size_t i = 0; i <= hlen - nlen; i++) {
        size_t j = 0;
        while (j < nlen && tolower((unsigned char)haystack[i + j]) == tolower((unsigned char)needle[j])) j++;
        if (j == nlen) return 1;
    }
    return 0;
}

/* ========================================================================= */
/* API DISPATCHER                                                            */
/* ========================================================================= */

int handle_api_action(const char *action, const char *payload) {
    ensure_data_dir();
    seed_sample_data_if_empty();

    if (strcmp(action, "get_dashboard") == 0) {
        int total_titles = 0, total_copies = 0, available_copies = 0;
        FILE *bf = fopen(g_books_file, "rb");
        if (bf) {
            Book b;
            while (fread(&b, sizeof(Book), 1, bf) == 1) {
                total_titles++;
                total_copies += b.total_copies;
                available_copies += b.available_copies;
            }
            fclose(bf);
        }

        int total_members = 0;
        FILE *mf = fopen(g_members_file, "rb");
        if (mf) {
            Member m;
            while (fread(&m, sizeof(Member), 1, mf) == 1) {
                if (m.active) total_members++;
            }
            fclose(mf);
        }

        int active_loans = 0;
        FILE *lf = fopen(g_loans_file, "rb");
        if (lf) {
            Loan l;
            while (fread(&l, sizeof(Loan), 1, lf) == 1) {
                if (!l.returned) active_loans++;
            }
            fclose(lf);
        }

        printf("{\"success\":true,\"stats\":{"
               "\"total_titles\":%d,\"total_copies\":%d,\"available_copies\":%d,"
               "\"issued_copies\":%d,\"total_members\":%d,\"active_loans\":%d}}\n",
               total_titles, total_copies, available_copies,
               total_copies - available_copies, total_members, active_loans);
        return 1;
    }

    if (strcmp(action, "list_books") == 0) {
        char query[TEXT_SIZE] = "", category[TEXT_SIZE] = "";
        if (payload) {
            json_str(payload, "query", query, sizeof(query));
            json_str(payload, "category", category, sizeof(category));
        }

        FILE *f = fopen(g_books_file, "rb");
        printf("{\"success\":true,\"books\":[");
        if (f) {
            Book b;
            int first = 1;
            while (fread(&b, sizeof(Book), 1, f) == 1) {
                char id_str[16];
                snprintf(id_str, sizeof(id_str), "%d", b.id);
                if (strlen(query) > 0 &&
                    !match_str(b.title, query) &&
                    !match_str(b.author, query) &&
                    !match_str(b.category, query) &&
                    !match_str(id_str, query)) {
                    continue;
                }
                if (strlen(category) > 0 && strcmp(category, "all") != 0 && strcasecmp(b.category, category) != 0) {
                    continue;
                }

                if (!first) printf(",");
                first = 0;

                char t[TEXT_SIZE*2], a[TEXT_SIZE*2], c[TEXT_SIZE*2];
                json_esc(b.title, t, sizeof(t));
                json_esc(b.author, a, sizeof(a));
                json_esc(b.category, c, sizeof(c));

                printf("{\"id\":%d,\"title\":\"%s\",\"author\":\"%s\",\"category\":\"%s\","
                       "\"total_copies\":%d,\"available_copies\":%d}",
                       b.id, t, a, c, b.total_copies, b.available_copies);
            }
            fclose(f);
        }
        printf("]}\n");
        return 1;
    }

    if (strcmp(action, "add_book") == 0) {
        Book b = {0};
        json_int(payload, "id", &b.id);
        json_str(payload, "title", b.title, sizeof(b.title));
        json_str(payload, "author", b.author, sizeof(b.author));
        json_str(payload, "category", b.category, sizeof(b.category));
        json_int(payload, "total_copies", &b.total_copies);

        char err[DESC_SIZE] = "";
        if (add_book_record(&b, err, sizeof(err))) {
            printf("{\"success\":true,\"message\":\"Book added successfully.\"}\n");
        } else {
            char esc[DESC_SIZE*2];
            json_esc(err, esc, sizeof(esc));
            printf("{\"success\":false,\"error\":\"%s\"}\n", esc);
        }
        return 1;
    }

    if (strcmp(action, "edit_book") == 0) {
        Book b = {0};
        json_int(payload, "id", &b.id);
        json_str(payload, "title", b.title, sizeof(b.title));
        json_str(payload, "author", b.author, sizeof(b.author));
        json_str(payload, "category", b.category, sizeof(b.category));
        json_int(payload, "total_copies", &b.total_copies);

        char err[DESC_SIZE] = "";
        if (edit_book_record(&b, err, sizeof(err))) {
            printf("{\"success\":true,\"message\":\"Book updated successfully.\"}\n");
        } else {
            char esc[DESC_SIZE*2];
            json_esc(err, esc, sizeof(esc));
            printf("{\"success\":false,\"error\":\"%s\"}\n", esc);
        }
        return 1;
    }

    if (strcmp(action, "delete_book") == 0) {
        int id = 0;
        json_int(payload, "id", &id);
        char err[DESC_SIZE] = "";
        if (delete_book_record(id, err, sizeof(err))) {
            printf("{\"success\":true,\"message\":\"Book deleted successfully.\"}\n");
        } else {
            char esc[DESC_SIZE*2];
            json_esc(err, esc, sizeof(esc));
            printf("{\"success\":false,\"error\":\"%s\"}\n", esc);
        }
        return 1;
    }

    if (strcmp(action, "list_members") == 0) {
        char query[TEXT_SIZE] = "";
        if (payload) json_str(payload, "query", query, sizeof(query));

        FILE *f = fopen(g_members_file, "rb");
        printf("{\"success\":true,\"members\":[");
        if (f) {
            Member m;
            int first = 1;
            while (fread(&m, sizeof(Member), 1, f) == 1) {
                if (!m.active) continue;
                char id_str[16];
                snprintf(id_str, sizeof(id_str), "%d", m.id);
                if (strlen(query) > 0 &&
                    !match_str(m.name, query) &&
                    !match_str(m.email, query) &&
                    !match_str(m.phone, query) &&
                    !match_str(m.department, query) &&
                    !match_str(id_str, query)) {
                    continue;
                }

                if (!first) printf(",");
                first = 0;

                char n[TEXT_SIZE*2], em[TEXT_SIZE*2], ph[TEXT_SIZE*2], dept[TEXT_SIZE*2];
                json_esc(m.name, n, sizeof(n));
                json_esc(m.email, em, sizeof(em));
                json_esc(m.phone, ph, sizeof(ph));
                json_esc(m.department, dept, sizeof(dept));

                int loans = get_member_loan_count(m.id);
                printf("{\"id\":%d,\"name\":\"%s\",\"email\":\"%s\",\"phone\":\"%s\",\"department\":\"%s\",\"active_loans\":%d}",
                       m.id, n, em, ph, dept, loans);
            }
            fclose(f);
        }
        printf("]}\n");
        return 1;
    }

    if (strcmp(action, "add_member") == 0) {
        Member m = {0};
        json_int(payload, "id", &m.id);
        json_str(payload, "name", m.name, sizeof(m.name));
        json_str(payload, "email", m.email, sizeof(m.email));
        json_str(payload, "phone", m.phone, sizeof(m.phone));
        json_str(payload, "department", m.department, sizeof(m.department));

        char err[DESC_SIZE] = "";
        if (add_member_record(&m, err, sizeof(err))) {
            printf("{\"success\":true,\"message\":\"Member registered successfully.\"}\n");
        } else {
            char esc[DESC_SIZE*2];
            json_esc(err, esc, sizeof(esc));
            printf("{\"success\":false,\"error\":\"%s\"}\n", esc);
        }
        return 1;
    }

    if (strcmp(action, "edit_member") == 0) {
        Member m = {0};
        json_int(payload, "id", &m.id);
        json_str(payload, "name", m.name, sizeof(m.name));
        json_str(payload, "email", m.email, sizeof(m.email));
        json_str(payload, "phone", m.phone, sizeof(m.phone));
        json_str(payload, "department", m.department, sizeof(m.department));

        char err[DESC_SIZE] = "";
        if (edit_member_record(&m, err, sizeof(err))) {
            printf("{\"success\":true,\"message\":\"Member updated successfully.\"}\n");
        } else {
            char esc[DESC_SIZE*2];
            json_esc(err, esc, sizeof(esc));
            printf("{\"success\":false,\"error\":\"%s\"}\n", esc);
        }
        return 1;
    }

    if (strcmp(action, "delete_member") == 0) {
        int id = 0;
        json_int(payload, "id", &id);
        char err[DESC_SIZE] = "";
        if (delete_member_record(id, err, sizeof(err))) {
            printf("{\"success\":true,\"message\":\"Member deleted successfully.\"}\n");
        } else {
            char esc[DESC_SIZE*2];
            json_esc(err, esc, sizeof(esc));
            printf("{\"success\":false,\"error\":\"%s\"}\n", esc);
        }
        return 1;
    }

    if (strcmp(action, "list_loans") == 0) {
        FILE *f = fopen(g_loans_file, "rb");
        printf("{\"success\":true,\"loans\":[");
        if (f) {
            Loan l;
            int first = 1;
            while (fread(&l, sizeof(Loan), 1, f) == 1) {
                if (l.returned) continue; /* Only active loans */

                Book b = {0};
                get_book_by_id(l.book_id, &b);
                Member m = {0};
                get_member_by_id(l.member_id, &m);

                if (!first) printf(",");
                first = 0;

                char bt[TEXT_SIZE*2], mn[TEXT_SIZE*2];
                json_esc(b.title[0] ? b.title : "Unknown Book", bt, sizeof(bt));
                json_esc(m.name[0] ? m.name : "Unknown Member", mn, sizeof(mn));

                printf("{\"id\":%d,\"book_id\":%d,\"book_title\":\"%s\","
                       "\"member_id\":%d,\"member_name\":\"%s\",\"returned\":%d}",
                       l.id, l.book_id, bt, l.member_id, mn, l.returned);
            }
            fclose(f);
        }
        printf("]}\n");
        return 1;
    }

    if (strcmp(action, "issue_book") == 0) {
        int book_id = 0, member_id = 0;
        json_int(payload, "book_id", &book_id);
        json_int(payload, "member_id", &member_id);

        char err[DESC_SIZE] = "";
        if (issue_book_record(book_id, member_id, err, sizeof(err))) {
            printf("{\"success\":true,\"message\":\"Book issued successfully.\"}\n");
        } else {
            char esc[DESC_SIZE*2];
            json_esc(err, esc, sizeof(esc));
            printf("{\"success\":false,\"error\":\"%s\"}\n", esc);
        }
        return 1;
    }

    if (strcmp(action, "return_book") == 0) {
        int loan_id = 0;
        json_int(payload, "loan_id", &loan_id);

        char err[DESC_SIZE] = "";
        if (return_book_record(loan_id, err, sizeof(err))) {
            printf("{\"success\":true,\"message\":\"Book returned successfully.\"}\n");
        } else {
            char esc[DESC_SIZE*2];
            json_esc(err, esc, sizeof(esc));
            printf("{\"success\":false,\"error\":\"%s\"}\n", esc);
        }
        return 1;
    }

    if (strcmp(action, "reset_demo") == 0) {
        remove(g_books_file);
        remove(g_members_file);
        remove(g_loans_file);
        seed_sample_data_if_empty();
        printf("{\"success\":true,\"message\":\"Library reset to default sample data.\"}\n");
        return 1;
    }

    printf("{\"success\":false,\"error\":\"Unknown action.\"}\n");
    return 0;
}

/* ========================================================================= */
/* ORIGINAL INTERACTIVE TERMINAL MENU                                        */
/* ========================================================================= */

static int read_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (!fgets(buffer, (int)size, stdin)) return 0;
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
    return 1;
}

static int read_int(const char *prompt, int *value) {
    char input[100];
    for (;;) {
        if (!read_line(prompt, input, sizeof(input))) return 0;
        char *end;
        long n = strtol(input, &end, 10);
        while (isspace((unsigned char)*end)) end++;
        if (input[0] != '\0' && *end == '\0') {
            *value = (int)n;
            return 1;
        }
        printf("Please enter a valid whole number.\n");
    }
}

static void cli_add_book(void) {
    Book b = {0};
    if (!read_int("Enter Book ID: ", &b.id)) return;
    if (!read_line("Enter Title: ", b.title, sizeof(b.title))) return;
    if (!read_line("Enter Author: ", b.author, sizeof(b.author))) return;
    if (!read_line("Enter Category: ", b.category, sizeof(b.category))) return;
    if (!read_int("Enter Total Copies: ", &b.total_copies)) return;

    char err[DESC_SIZE] = "";
    if (add_book_record(&b, err, sizeof(err))) {
        printf(">>> Book added successfully.\n");
    } else {
        printf(">>> Error: %s\n", err);
    }
}

static void cli_list_books(void) {
    FILE *f = fopen(g_books_file, "rb");
    if (!f) {
        printf("No books found.\n");
        return;
    }
    Book b;
    printf("\n%-6s %-26s %-20s %-16s %-8s %-8s\n", "ID", "Title", "Author", "Category", "Total", "Avail");
    printf("----------------------------------------------------------------------------------------\n");
    while (fread(&b, sizeof(Book), 1, f) == 1) {
        printf("%-6d %-26.26s %-20.20s %-16.16s %-8d %-8d\n",
               b.id, b.title, b.author, b.category, b.total_copies, b.available_copies);
    }
    fclose(f);
}

static void cli_search_books(void) {
    char query[TEXT_SIZE];
    if (!read_line("Enter search query: ", query, sizeof(query))) return;
    FILE *f = fopen(g_books_file, "rb");
    if (!f) return;
    Book b;
    int found = 0;
    while (fread(&b, sizeof(Book), 1, f) == 1) {
        if (match_str(b.title, query) || match_str(b.author, query)) {
            printf("\nID: %d | Title: %s | Author: %s | Avail: %d/%d\n",
                   b.id, b.title, b.author, b.available_copies, b.total_copies);
            found = 1;
        }
    }
    if (!found) printf("No matching books found.\n");
    fclose(f);
}

static void cli_issue_book(void) {
    int book_id, member_id;
    if (!read_int("Enter Book ID to issue: ", &book_id)) return;
    if (!read_int("Enter Member ID: ", &member_id)) return;
    char err[DESC_SIZE] = "";
    if (issue_book_record(book_id, member_id, err, sizeof(err))) {
        printf(">>> Book issued successfully.\n");
    } else {
        printf(">>> Error: %s\n", err);
    }
}

static void cli_return_book(void) {
    int loan_id;
    if (!read_int("Enter Loan ID to return: ", &loan_id)) return;
    char err[DESC_SIZE] = "";
    if (return_book_record(loan_id, err, sizeof(err))) {
        printf(">>> Book returned successfully.\n");
    } else {
        printf(">>> Error: %s\n", err);
    }
}

void interactive_menu(void) {
    ensure_data_dir();
    seed_sample_data_if_empty();

    int choice;
    for (;;) {
        printf("\n=========================================\n");
        printf("   LIBRARY MANAGEMENT SYSTEM (C CORE)   \n");
        printf("=========================================\n");
        printf("1. Add a book\n");
        printf("2. List all books\n");
        printf("3. Search books\n");
        printf("4. Issue a book\n");
        printf("5. Return a book\n");
        printf("0. Exit\n");
        printf("-----------------------------------------\n");

        if (!read_int("Choose an option: ", &choice)) break;

        switch (choice) {
            case 1: cli_add_book(); break;
            case 2: cli_list_books(); break;
            case 3: cli_search_books(); break;
            case 4: cli_issue_book(); break;
            case 5: cli_return_book(); break;
            case 0: printf("Goodbye.\n"); return;
            default: printf("Invalid choice.\n");
        }
    }
}

/* ========================================================================= */
/* MAIN ENTRY POINT                                                          */
/* ========================================================================= */

int main(int argc, char *argv[]) {
    init_paths();
    ensure_data_dir();

    if (argc >= 3 && strcmp(argv[1], "--api") == 0) {
        const char *action = argv[2];
        char payload[4096] = {0};
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
                safe_copy(payload, argv[i + 1], sizeof(payload));
                break;
            } else if (payload[0] == '\0') {
                safe_copy(payload, argv[i], sizeof(payload));
            }
        }
        handle_api_action(action, payload);
        return 0;
    }

    interactive_menu();
    return 0;
}
