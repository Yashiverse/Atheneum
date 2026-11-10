# Athenaeum - Library Management System (Node.js & C Engine)

A clean, responsive web interface built on top of a streamlined **C Library Management System**, powered by **Node.js** (JavaScript) with **zero external npm dependencies**.

---

## Features

1. **Dashboard**
   - **Total Books**: Distinct titles cataloged in `books.dat`.
   - **Available Copies**: Copies on the shelf ready for borrowing.
   - **Issued Copies**: Total copies currently borrowed.
   - **Total Members**: Registered students and faculty in `members.dat`.
   - **Active Loans Table**: Quick list of who has which book with a 1-click **Return** button.

2. **Book Management**
   - View all books in a searchable, sortable table (by ID, Title, Author, Category).
   - Add new books with ID uniqueness and copy validation.
   - Edit book title, author, category, and total copies.
   - Delete a book (safely blocked by C backend if any copies are currently issued out).
   - Instant search bar and category filter dropdown.

3. **Member Management**
   - View members with ID, Name, Email, Phone, Department, and Active Borrow count.
   - Register new members.
   - Edit member information.
   - Remove member (blocked by C backend if member has unreturned books).
   - Real-time search by name, ID, or department.

4. **Issue & Return**
   - **Issue Book**: Select an available book and a member &rarr; click **Issue Book**.
     - C core verifies availability > 0, decrements available copies in `books.dat`, and records the loan in `loans.dat`.
   - **Return Book**: Click **Return Book** &rarr; C core increments available copies in `books.dat` and marks the loan completed.

---

## Architecture

```
  [ Web Browser ]
        │  HTML / CSS / JavaScript
        ▼
  [ server.js ]  (Node.js built-ins: http, fs, child_process - Zero npm packages required)
        │  Subprocess execution
        ▼
  [ backend/library_core.exe ]  (Compiled C program)
        │  Direct binary disk I/O (fread, fwrite, fseek)
        ▼
  ┌─────────────┐   ┌───────────────┐   ┌─────────────┐
  │  books.dat  │   │  members.dat  │   │  loans.dat  │
  └─────────────┘   └───────────────┘   └─────────────┘
```

---

## How to Run in Antigravity IDE

1. **Open the Terminal Panel**:
   Press **<kbd>Ctrl</kbd> + <kbd>`</kbd>** (backtick key).
2. **Start the Server with Node.js**:
   ```bash
   node server.js
   ```
3. **Open the Web Application**:
   Hold **<kbd>Ctrl</kbd>** and click:
   👉 **`http://localhost:8080`**

*(To stop the server at any time, press **<kbd>Ctrl</kbd> + <kbd>C</kbd>** in the terminal).*

---

## Running from Windows (1-Click)

Double-click **`run.bat`** in the project folder. It will launch the Node.js server and automatically open your browser.

---

## Running the Classic C Console Menu

In the terminal:
```powershell
cd backend
.\library_core.exe
```
This runs the original interactive C console menu. Any changes made in the C terminal menu appear immediately in the web browser, and vice versa!
