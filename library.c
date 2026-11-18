/*
 * Library Management System (department-based access)
 *
 * Admin   : can view ALL books, add / delete books, update stock,
 *           view inventory summary, register / view students.
 * Student : can see and search ONLY books of their own department,
 *           and issue / return books from that department.
 *
 * Default admin login -> username: admin   password: admin123
 * Data is saved in books.dat and students.dat (created automatically).
 *
 * Compile: gcc library.c -o library
 * Run    : ./library
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOOKS    500
#define MAX_STUDENTS 200
#define MAX_ISSUE    3
#define BOOK_FILE    "books.dat"
#define STUDENT_FILE "students.dat"
#define ADMIN_USER   "admin"
#define ADMIN_PASS   "admin123"

#define NUM_DEPTS 6
const char *DEPT_NAMES[NUM_DEPTS] = {
    "Mathematics", "Computer Science", "Physics",
    "Chemistry", "Electronics", "Mechanical"
};

typedef struct {
    int  id;
    char title[80];
    char author[60];
    int  dept;      /* index into DEPT_NAMES */
    int  qty;       /* copies currently in stock */
} Book;

typedef struct {
    int  id;
    char name[50];
    char password[20];
    int  dept;
    int  issued[MAX_ISSUE];   /* book ids, 0 = empty slot */
} Student;

Book    books[MAX_BOOKS];
Student students[MAX_STUDENTS];
int     bookCount = 0, studentCount = 0;

/* ---------- input helpers ---------- */

void readStr(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    if (!fgets(buf, size, stdin)) { buf[0] = '\0'; return; }
    buf[strcspn(buf, "\n")] = '\0';
}

int readInt(const char *prompt) {
    char tmp[32];
    char *end;
    long v;
    while (1) {
        readStr(prompt, tmp, sizeof tmp);
        v = strtol(tmp, &end, 10);
        if (end != tmp && *end == '\0') return (int)v;
        printf("  Please enter a valid number.\n");
    }
}

int pickDepartment(void) {
    int i, c;
    printf("\nDepartments:\n");
    for (i = 0; i < NUM_DEPTS; i++) printf("  %d. %s\n", i + 1, DEPT_NAMES[i]);
    do {
        c = readInt("Choose department: ");
    } while (c < 1 || c > NUM_DEPTS);
    return c - 1;
}

int containsIgnoreCase(const char *text, const char *word) {
    char a[200], b[200];
    int i;
    for (i = 0; text[i] && i < 199; i++) a[i] = tolower((unsigned char)text[i]);
    a[i] = '\0';
    for (i = 0; word[i] && i < 199; i++) b[i] = tolower((unsigned char)word[i]);
    b[i] = '\0';
    return strstr(a, b) != NULL;
}

/* ---------- file handling ---------- */

void loadData(void) {
    FILE *f = fopen(BOOK_FILE, "rb");
    if (f) { bookCount = (int)fread(books, sizeof(Book), MAX_BOOKS, f); fclose(f); }
    f = fopen(STUDENT_FILE, "rb");
    if (f) { studentCount = (int)fread(students, sizeof(Student), MAX_STUDENTS, f); fclose(f); }
}

void saveBooks(void) {
    FILE *f = fopen(BOOK_FILE, "wb");
    if (!f) { printf("Error: cannot write %s\n", BOOK_FILE); return; }
    fwrite(books, sizeof(Book), bookCount, f);
    fclose(f);
}

void saveStudents(void) {
    FILE *f = fopen(STUDENT_FILE, "wb");
    if (!f) { printf("Error: cannot write %s\n", STUDENT_FILE); return; }
    fwrite(students, sizeof(Student), studentCount, f);
    fclose(f);
}

/* ---------- lookup ---------- */

int findBook(int id) {
    int i;
    for (i = 0; i < bookCount; i++) if (books[i].id == id) return i;
    return -1;
}

int findStudent(int id) {
    int i;
    for (i = 0; i < studentCount; i++) if (students[i].id == id) return i;
    return -1;
}

void printHeader(void) {
    printf("\n%-5s %-32s %-22s %-18s %s\n", "ID", "Title", "Author", "Department", "Stock");
    printf("---------------------------------------------------------------------------------------\n");
}

void printBook(const Book *b) {
    printf("%-5d %-32.32s %-22.22s %-18s %d\n",
           b->id, b->title, b->author, DEPT_NAMES[b->dept], b->qty);
}

/* ---------- admin functions ---------- */

void addBook(void) {
    Book b;
    if (bookCount >= MAX_BOOKS) { printf("Library is full!\n"); return; }

    b.id = readInt("Book ID: ");
    if (b.id <= 0) { printf("ID must be positive.\n"); return; }
    if (findBook(b.id) != -1) { printf("A book with this ID already exists.\n"); return; }

    readStr("Title : ", b.title, sizeof b.title);
    readStr("Author: ", b.author, sizeof b.author);
    b.dept = pickDepartment();
    b.qty  = readInt("Number of copies: ");
    if (b.qty < 0) { printf("Copies cannot be negative.\n"); return; }

    books[bookCount++] = b;
    saveBooks();
    printf("Book added successfully.\n");
}

void deleteBook(void) {
    int id = readInt("Enter ID of book to delete: ");
    int i = findBook(id);
    char ans[8];
    if (i == -1) { printf("Book not found.\n"); return; }

    printHeader();
    printBook(&books[i]);
    readStr("Delete this book? (y/n): ", ans, sizeof ans);
    if (tolower((unsigned char)ans[0]) != 'y') { printf("Cancelled.\n"); return; }

    for (; i < bookCount - 1; i++) books[i] = books[i + 1];
    bookCount--;
    saveBooks();
    printf("Book deleted.\n");
}

void viewAllBooks(void) {
    int i;
    if (bookCount == 0) { printf("No books in the library.\n"); return; }
    printHeader();
    for (i = 0; i < bookCount; i++) printBook(&books[i]);
}

void updateStock(void) {
    int id = readInt("Enter book ID: ");
    int i = findBook(id), change;
    if (i == -1) { printf("Book not found.\n"); return; }
    printf("'%s' currently has %d copies.\n", books[i].title, books[i].qty);
    change = readInt("Enter change (+ to add copies, - to remove): ");
    if (books[i].qty + change < 0) { printf("Stock cannot go below zero.\n"); return; }
    books[i].qty += change;
    saveBooks();
    printf("Stock updated. New stock: %d\n", books[i].qty);
}

void inventorySummary(void) {
    int d, i;
    printf("\n%-20s %-10s %s\n", "Department", "Titles", "Total copies");
    printf("------------------------------------------\n");
    for (d = 0; d < NUM_DEPTS; d++) {
        int titles = 0, copies = 0;
        for (i = 0; i < bookCount; i++)
            if (books[i].dept == d) { titles++; copies += books[i].qty; }
        printf("%-20s %-10d %d\n", DEPT_NAMES[d], titles, copies);
    }
    printf("\nOut-of-stock books:\n");
    {
        int any = 0;
        for (i = 0; i < bookCount; i++)
            if (books[i].qty == 0) { printf("  [%d] %s\n", books[i].id, books[i].title); any = 1; }
        if (!any) printf("  None\n");
    }
}

void registerStudent(void) {
    Student s;
    int j;
    if (studentCount >= MAX_STUDENTS) { printf("Student list is full!\n"); return; }
    s.id = readInt("Student ID: ");
    if (s.id <= 0) { printf("ID must be positive.\n"); return; }
    if (findStudent(s.id) != -1) { printf("Student ID already exists.\n"); return; }
    readStr("Name    : ", s.name, sizeof s.name);
    readStr("Password: ", s.password, sizeof s.password);
    s.dept = pickDepartment();
    for (j = 0; j < MAX_ISSUE; j++) s.issued[j] = 0;
    students[studentCount++] = s;
    saveStudents();
    printf("Student registered.\n");
}

void viewStudents(void) {
    int i;
    if (studentCount == 0) { printf("No students registered.\n"); return; }
    printf("\n%-8s %-25s %-20s %s\n", "ID", "Name", "Department", "Books issued");
    printf("----------------------------------------------------------------\n");
    for (i = 0; i < studentCount; i++) {
        int j, n = 0;
        for (j = 0; j < MAX_ISSUE; j++) if (students[i].issued[j]) n++;
        printf("%-8d %-25.25s %-20s %d\n",
               students[i].id, students[i].name, DEPT_NAMES[students[i].dept], n);
    }
}

void adminMenu(void) {
    int ch;
    do {
        printf("\n===== ADMIN MENU =====\n");
        printf("1. Add book\n2. Delete book\n3. View all books (inventory)\n");
        printf("4. Update stock\n5. Inventory summary\n");
        printf("6. Register student\n7. View students\n0. Logout\n");
        ch = readInt("Choice: ");
        switch (ch) {
            case 1: addBook(); break;
            case 2: deleteBook(); break;
            case 3: viewAllBooks(); break;
            case 4: updateStock(); break;
            case 5: inventorySummary(); break;
            case 6: registerStudent(); break;
            case 7: viewStudents(); break;
            case 0: printf("Logged out.\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (ch != 0);
}

/* ---------- student functions ---------- */

void viewDeptBooks(const Student *s) {
    int i, found = 0;
    printf("\nBooks available for %s department:", DEPT_NAMES[s->dept]);
    printHeader();
    for (i = 0; i < bookCount; i++)
        if (books[i].dept == s->dept) { printBook(&books[i]); found = 1; }
    if (!found) printf("No books available in your department yet.\n");
}

void searchDeptBooks(const Student *s) {
    char key[60];
    int i, found = 0;
    readStr("Search title/author: ", key, sizeof key);
    printHeader();
    for (i = 0; i < bookCount; i++)
        if (books[i].dept == s->dept &&
            (containsIgnoreCase(books[i].title, key) || containsIgnoreCase(books[i].author, key))) {
            printBook(&books[i]);
            found = 1;
        }
    if (!found) printf("No matching books in your department.\n");
}

void issueBook(Student *s) {
    int id = readInt("Enter ID of book to issue: ");
    int i = findBook(id), j;
    if (i == -1) { printf("Book not found.\n"); return; }
    if (books[i].dept != s->dept) {
        printf("Access denied: this book belongs to another department.\n");
        return;
    }
    if (books[i].qty <= 0) { printf("Sorry, no copies available.\n"); return; }
    for (j = 0; j < MAX_ISSUE; j++)
        if (s->issued[j] == id) { printf("You already have this book.\n"); return; }
    for (j = 0; j < MAX_ISSUE; j++) {
        if (s->issued[j] == 0) {
            s->issued[j] = id;
            books[i].qty--;
            saveBooks(); saveStudents();
            printf("Book issued: %s\n", books[i].title);
            return;
        }
    }
    printf("You can only hold %d books at a time. Return one first.\n", MAX_ISSUE);
}

void returnBook(Student *s) {
    int id = readInt("Enter ID of book to return: ");
    int j;
    for (j = 0; j < MAX_ISSUE; j++) {
        if (s->issued[j] == id) {
            int i = findBook(id);
            s->issued[j] = 0;
            if (i != -1) books[i].qty++;
            saveBooks(); saveStudents();
            printf("Book returned. Thank you!\n");
            return;
        }
    }
    printf("You have not issued a book with that ID.\n");
}

void myBooks(const Student *s) {
    int j, any = 0;
    printf("\nBooks currently issued to you:\n");
    for (j = 0; j < MAX_ISSUE; j++) {
        if (s->issued[j]) {
            int i = findBook(s->issued[j]);
            if (i != -1) printf("  [%d] %s by %s\n", books[i].id, books[i].title, books[i].author);
            else         printf("  [%d] (book removed from library)\n", s->issued[j]);
            any = 1;
        }
    }
    if (!any) printf("  None\n");
}

void studentMenu(Student *s) {
    int ch;
    printf("\nWelcome, %s (%s department)\n", s->name, DEPT_NAMES[s->dept]);
    do {
        printf("\n===== STUDENT MENU =====\n");
        printf("1. View my department books\n2. Search my department books\n");
        printf("3. Issue book\n4. Return book\n5. My issued books\n0. Logout\n");
        ch = readInt("Choice: ");
        switch (ch) {
            case 1: viewDeptBooks(s); break;
            case 2: searchDeptBooks(s); break;
            case 3: issueBook(s); break;
            case 4: returnBook(s); break;
            case 5: myBooks(s); break;
            case 0: printf("Logged out.\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (ch != 0);
}

/* ---------- login ---------- */

void adminLogin(void) {
    char u[30], p[30];
    readStr("Username: ", u, sizeof u);
    readStr("Password: ", p, sizeof p);
    if (strcmp(u, ADMIN_USER) == 0 && strcmp(p, ADMIN_PASS) == 0) adminMenu();
    else printf("Invalid admin credentials.\n");
}

void studentLogin(void) {
    int id = readInt("Student ID: ");
    char p[30];
    int i = findStudent(id);
    readStr("Password: ", p, sizeof p);
    if (i != -1 && strcmp(students[i].password, p) == 0) studentMenu(&students[i]);
    else printf("Invalid student ID or password.\n");
}

int main(void) {
    int ch;
    loadData();
    do {
        printf("\n===== LIBRARY MANAGEMENT SYSTEM =====\n");
        printf("1. Admin login\n2. Student login\n0. Exit\n");
        ch = readInt("Choice: ");
        switch (ch) {
            case 1: adminLogin(); break;
            case 2: studentLogin(); break;
            case 0: printf("Goodbye!\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (ch != 0);
    return 0;
}
