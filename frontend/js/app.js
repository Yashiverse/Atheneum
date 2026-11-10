/**
 * app.js - Streamlined Application Controller
 * Handles UI interactions and connects to the C backend REST API.
 */

const state = {
  currentView: 'dashboard',
  stats: {},
  books: [],
  members: [],
  loans: [],
  bookSort: { column: 'id', order: 'asc' },
  bookCategoryFilter: 'all',
  bookSearchQuery: '',
  memberSearchQuery: '',
  activeModal: null
};

/* ==========================================================================
   API CLIENT
   ========================================================================== */

async function apiRequest(endpoint, options = {}) {
  try {
    const res = await fetch(endpoint, {
      ...options,
      headers: { 'Content-Type': 'application/json', ...(options.headers || {}) }
    });
    return await res.json();
  } catch (err) {
    console.error(`API Error on ${endpoint}:`, err);
    return { success: false, error: 'Network error connecting to backend.' };
  }
}

/* ==========================================================================
   TOAST NOTIFICATIONS
   ========================================================================== */

function showToast(message, type = 'info', duration = 3500) {
  const container = document.getElementById('toast-container');
  if (!container) return;

  const toast = document.createElement('div');
  toast.className = `toast toast-${type}`;
  toast.innerHTML = `
    <div class="toast-body">${escapeHTML(message)}</div>
    <button class="toast-close" onclick="this.parentElement.remove()">&times;</button>
  `;
  container.appendChild(toast);

  setTimeout(() => {
    toast.style.opacity = '0';
    setTimeout(() => toast.remove(), 300);
  }, duration);
}

function escapeHTML(str) {
  if (!str) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

/* ==========================================================================
   MODAL CONTROLS
   ========================================================================== */

function openModal(id) {
  const m = document.getElementById(id);
  if (!m) return;
  m.classList.add('open');
  state.activeModal = id;
}

function closeModal(id) {
  const m = document.getElementById(id);
  if (!m) return;
  m.classList.remove('open');
  state.activeModal = null;
}

window.addEventListener('keydown', (e) => {
  if (e.key === 'Escape' && state.activeModal) {
    closeModal(state.activeModal);
  }
});

document.addEventListener('click', (e) => {
  if (e.target.classList.contains('modal-backdrop')) {
    closeModal(e.target.id);
  }
});

/* ==========================================================================
   ROUTING
   ========================================================================== */

function navigateTo(viewId) {
  state.currentView = viewId;
  window.location.hash = viewId;

  document.querySelectorAll('.nav-item').forEach(el => {
    el.classList.toggle('active', el.getAttribute('data-view') === viewId);
  });

  document.querySelectorAll('.view-panel').forEach(panel => {
    panel.classList.toggle('active-view', panel.id === `view-${viewId}`);
  });

  const titles = {
    dashboard: { title: 'Dashboard', sub: 'Overview of books, circulation, and members' },
    books: { title: 'Book Management', sub: 'Search, catalog, edit, and manage library titles' },
    members: { title: 'Member Management', sub: 'Register and manage library members' },
    transactions: { title: 'Issue & Return', sub: 'Issue books and process returns' }
  };

  const info = titles[viewId] || titles.dashboard;
  document.getElementById('page-main-title').textContent = info.title;
  document.getElementById('page-sub-title').textContent = info.sub;

  const sidebar = document.getElementById('sidebar');
  if (sidebar) sidebar.classList.remove('mobile-open');

  refreshCurrentView();
}

function refreshCurrentView() {
  if (state.currentView === 'dashboard') loadDashboard();
  else if (state.currentView === 'books') loadBooks();
  else if (state.currentView === 'members') loadMembers();
  else if (state.currentView === 'transactions') loadLoans();
}

/* ==========================================================================
   1. DASHBOARD
   ========================================================================== */

async function loadDashboard() {
  const res = await apiRequest('/api/dashboard');
  if (!res.success) {
    showToast(res.error || 'Failed to load dashboard data', 'error');
    return;
  }

  state.stats = res.stats || {};
  document.getElementById('stat-total-books').textContent = state.stats.total_titles || 0;
  document.getElementById('stat-available-copies').textContent = state.stats.available_copies || 0;
  document.getElementById('stat-issued-copies').textContent = state.stats.issued_copies || 0;
  document.getElementById('stat-total-members').textContent = state.stats.total_members || 0;

  renderDashboardLoans();
}

async function renderDashboardLoans() {
  const container = document.getElementById('dashboard-mini-loans');
  if (!container) return;

  const res = await apiRequest('/api/loans');
  const loans = res.loans || [];

  if (loans.length === 0) {
    container.innerHTML = `
      <div class="state-container" style="padding: 24px;">
        <div class="state-desc">No active book loans at this moment.</div>
      </div>
    `;
    return;
  }

  container.innerHTML = `
    <table class="data-table">
      <thead>
        <tr>
          <th>Book Title</th>
          <th>Borrower</th>
          <th>Status</th>
          <th style="text-align: right;">Action</th>
        </tr>
      </thead>
      <tbody>
        ${loans.slice(0, 6).map(l => `
          <tr>
            <td><strong>${escapeHTML(l.book_title)}</strong></td>
            <td>${escapeHTML(l.member_name)}</td>
            <td><span class="badge badge-warning">On Loan</span></td>
            <td style="text-align: right;">
              <button class="btn btn-sm btn-secondary" onclick="promptReturnBook(${l.id}, '${escapeHTML(l.book_title)}')">Return</button>
            </td>
          </tr>
        `).join('')}
      </tbody>
    </table>
  `;
}

/* ==========================================================================
   2. BOOK MANAGEMENT
   ========================================================================== */

async function loadBooks() {
  const tbody = document.getElementById('books-table-body');
  if (tbody) {
    tbody.innerHTML = `<tr><td colspan="7" class="state-container"><div class="spinner"></div></td></tr>`;
  }

  const query = state.bookSearchQuery.trim();
  const cat = state.bookCategoryFilter;
  const res = await apiRequest(`/api/books?query=${encodeURIComponent(query)}&category=${encodeURIComponent(cat)}`);

  if (!res.success) {
    showToast(res.error || 'Failed to load books', 'error');
    return;
  }

  state.books = res.books || [];
  populateCategoryDropdown();
  renderBooksTable();
}

function populateCategoryDropdown() {
  const select = document.getElementById('filter-book-category');
  if (!select) return;
  const current = select.value;
  const categories = Array.from(new Set(state.books.map(b => b.category).filter(Boolean))).sort();

  select.innerHTML = '<option value="all">All Categories</option>' +
    categories.map(c => `<option value="${escapeHTML(c)}" ${c === current ? 'selected' : ''}>${escapeHTML(c)}</option>`).join('');
}

function renderBooksTable() {
  const tbody = document.getElementById('books-table-body');
  if (!tbody) return;

  let list = [...state.books];
  const { column, order } = state.bookSort;

  list.sort((a, b) => {
    let valA = a[column], valB = b[column];
    if (typeof valA === 'string') valA = valA.toLowerCase();
    if (typeof valB === 'string') valB = valB.toLowerCase();
    if (valA < valB) return order === 'asc' ? -1 : 1;
    if (valA > valB) return order === 'asc' ? 1 : -1;
    return 0;
  });

  if (list.length === 0) {
    tbody.innerHTML = `
      <tr>
        <td colspan="7">
          <div class="state-container">
            <div class="state-title">No Books Found</div>
            <div class="state-desc">Try clearing your search query.</div>
            <button class="btn btn-gold btn-sm" onclick="openAddBookModal()">+ Add Book</button>
          </div>
        </td>
      </tr>
    `;
    return;
  }

  tbody.innerHTML = list.map(b => {
    const isAvail = b.available_copies > 0;
    const badge = isAvail
      ? `<span class="badge badge-success">${b.available_copies} Available</span>`
      : `<span class="badge badge-danger">Out of Stock</span>`;

    return `
      <tr>
        <td><code>#${b.id}</code></td>
        <td><strong>${escapeHTML(b.title)}</strong></td>
        <td>${escapeHTML(b.author)}</td>
        <td><span class="badge badge-neutral">${escapeHTML(b.category)}</span></td>
        <td>${b.available_copies} / ${b.total_copies}</td>
        <td>${badge}</td>
        <td style="text-align: right;">
          <div class="table-actions" style="justify-content: flex-end;">
            ${isAvail ? `
              <button class="action-btn btn-issue-row" title="Issue book" onclick="quickIssueBook(${b.id})">Issue</button>
            ` : ''}
            <button class="action-btn" title="Edit book" onclick="openEditBookModal(${b.id})">
              <svg width="15" height="15" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15.232 5.232l3.536 3.536m-2.036-5.036a2.5 2.5 0 113.536 3.536L6.5 21.036H3v-3.572L16.732 3.732z"></path></svg>
            </button>
            <button class="action-btn btn-delete-row" title="Delete book" onclick="promptDeleteBook(${b.id}, '${escapeHTML(b.title)}')">
              <svg width="15" height="15" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16"></path></svg>
            </button>
          </div>
        </td>
      </tr>
    `;
  }).join('');
}

function sortBooksBy(col) {
  if (state.bookSort.column === col) {
    state.bookSort.order = state.bookSort.order === 'asc' ? 'desc' : 'asc';
  } else {
    state.bookSort.column = col;
    state.bookSort.order = 'asc';
  }

  document.querySelectorAll('#books-table th.sortable').forEach(th => {
    th.classList.toggle('sorted', th.getAttribute('data-col') === col);
    const arrow = th.querySelector('.sort-arrow');
    if (arrow) arrow.textContent = th.getAttribute('data-col') === col ? (state.bookSort.order === 'asc' ? '↑' : '↓') : '↕';
  });

  renderBooksTable();
}

function openAddBookModal() {
  document.getElementById('form-add-book').reset();
  const maxId = state.books.reduce((max, b) => (b.id > max ? b.id : max), 100);
  document.getElementById('add-book-id').value = maxId + 1;
  openModal('modal-add-book');
}

async function handleAddBookSubmit(e) {
  e.preventDefault();
  const id = parseInt(document.getElementById('add-book-id').value, 10);
  const title = document.getElementById('add-book-title').value.trim();
  const author = document.getElementById('add-book-author').value.trim();
  const category = document.getElementById('add-book-category').value.trim() || 'General';
  const total_copies = parseInt(document.getElementById('add-book-copies').value, 10);

  const res = await apiRequest('/api/books', {
    method: 'POST',
    body: JSON.stringify({ id, title, author, category, total_copies })
  });

  if (res.success) {
    showToast(res.message || 'Book added successfully', 'success');
    closeModal('modal-add-book');
    loadBooks();
  } else {
    showToast(res.error || 'Failed to add book', 'error');
  }
}

function openEditBookModal(id) {
  const b = state.books.find(x => x.id === id);
  if (!b) return;

  document.getElementById('edit-book-id').value = b.id;
  document.getElementById('edit-book-title').value = b.title;
  document.getElementById('edit-book-author').value = b.author;
  document.getElementById('edit-book-category').value = b.category;
  document.getElementById('edit-book-copies').value = b.total_copies;

  openModal('modal-edit-book');
}

async function handleEditBookSubmit(e) {
  e.preventDefault();
  const id = parseInt(document.getElementById('edit-book-id').value, 10);
  const title = document.getElementById('edit-book-title').value.trim();
  const author = document.getElementById('edit-book-author').value.trim();
  const category = document.getElementById('edit-book-category').value.trim();
  const total_copies = parseInt(document.getElementById('edit-book-copies').value, 10);

  const res = await apiRequest(`/api/books/${id}`, {
    method: 'PUT',
    body: JSON.stringify({ title, author, category, total_copies })
  });

  if (res.success) {
    showToast('Book updated', 'success');
    closeModal('modal-edit-book');
    loadBooks();
  } else {
    showToast(res.error || 'Failed to update book', 'error');
  }
}

function promptDeleteBook(id, title) {
  document.getElementById('delete-book-id').value = id;
  document.getElementById('delete-book-title-display').textContent = `"${title}" (ID: ${id})`;
  openModal('modal-delete-book');
}

async function confirmDeleteBook() {
  const id = parseInt(document.getElementById('delete-book-id').value, 10);
  const res = await apiRequest(`/api/books/${id}`, { method: 'DELETE' });

  if (res.success) {
    showToast('Book deleted', 'success');
    closeModal('modal-delete-book');
    loadBooks();
  } else {
    showToast(res.error || 'Failed to delete book', 'error');
  }
}

/* ==========================================================================
   3. MEMBER MANAGEMENT
   ========================================================================== */

async function loadMembers() {
  const tbody = document.getElementById('members-table-body');
  if (tbody) {
    tbody.innerHTML = `<tr><td colspan="6" class="state-container"><div class="spinner"></div></td></tr>`;
  }

  const query = state.memberSearchQuery.trim();
  const res = await apiRequest(`/api/members?query=${encodeURIComponent(query)}`);

  if (!res.success) {
    showToast(res.error || 'Failed to load members', 'error');
    return;
  }

  state.members = res.members || [];
  renderMembersTable();
}

function renderMembersTable() {
  const tbody = document.getElementById('members-table-body');
  if (!tbody) return;

  if (state.members.length === 0) {
    tbody.innerHTML = `
      <tr>
        <td colspan="6">
          <div class="state-container">
            <div class="state-title">No Members Found</div>
            <button class="btn btn-primary btn-sm" onclick="openAddMemberModal()">+ Register Member</button>
          </div>
        </td>
      </tr>
    `;
    return;
  }

  tbody.innerHTML = state.members.map(m => `
    <tr>
      <td><code>#${m.id}</code></td>
      <td><strong>${escapeHTML(m.name)}</strong></td>
      <td>
        <div>${escapeHTML(m.email)}</div>
        <div style="font-size: 0.76rem; color: var(--text-muted);">${escapeHTML(m.phone)}</div>
      </td>
      <td><span class="badge badge-neutral">${escapeHTML(m.department)}</span></td>
      <td>
        <span class="badge ${m.active_loans > 0 ? 'badge-warning' : 'badge-neutral'}">
          ${m.active_loans} borrowed
        </span>
      </td>
      <td style="text-align: right;">
        <div class="table-actions" style="justify-content: flex-end;">
          <button class="action-btn" title="Edit member" onclick="openEditMemberModal(${m.id})">
            <svg width="15" height="15" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15.232 5.232l3.536 3.536m-2.036-5.036a2.5 2.5 0 113.536 3.536L6.5 21.036H3v-3.572L16.732 3.732z"></path></svg>
          </button>
          <button class="action-btn btn-delete-row" title="Remove member" onclick="promptDeleteMember(${m.id}, '${escapeHTML(m.name)}', ${m.active_loans})">
            <svg width="15" height="15" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16"></path></svg>
          </button>
        </div>
      </td>
    </tr>
  `).join('');
}

function openAddMemberModal() {
  document.getElementById('form-add-member').reset();
  const maxId = state.members.reduce((max, m) => (m.id > max ? m.id : max), 0);
  document.getElementById('add-member-id').value = maxId + 1;
  openModal('modal-add-member');
}

async function handleAddMemberSubmit(e) {
  e.preventDefault();
  const id = parseInt(document.getElementById('add-member-id').value, 10);
  const name = document.getElementById('add-member-name').value.trim();
  const email = document.getElementById('add-member-email').value.trim();
  const phone = document.getElementById('add-member-phone').value.trim();
  const department = document.getElementById('add-member-dept').value.trim();

  const res = await apiRequest('/api/members', {
    method: 'POST',
    body: JSON.stringify({ id, name, email, phone, department })
  });

  if (res.success) {
    showToast('Member registered', 'success');
    closeModal('modal-add-member');
    loadMembers();
  } else {
    showToast(res.error || 'Failed to register member', 'error');
  }
}

function openEditMemberModal(id) {
  const m = state.members.find(x => x.id === id);
  if (!m) return;

  document.getElementById('edit-member-id').value = m.id;
  document.getElementById('edit-member-name').value = m.name;
  document.getElementById('edit-member-email').value = m.email;
  document.getElementById('edit-member-phone').value = m.phone;
  document.getElementById('edit-member-dept').value = m.department;

  openModal('modal-edit-member');
}

async function handleEditMemberSubmit(e) {
  e.preventDefault();
  const id = parseInt(document.getElementById('edit-member-id').value, 10);
  const name = document.getElementById('edit-member-name').value.trim();
  const email = document.getElementById('edit-member-email').value.trim();
  const phone = document.getElementById('edit-member-phone').value.trim();
  const department = document.getElementById('edit-member-dept').value.trim();

  const res = await apiRequest(`/api/members/${id}`, {
    method: 'PUT',
    body: JSON.stringify({ name, email, phone, department })
  });

  if (res.success) {
    showToast('Member updated', 'success');
    closeModal('modal-edit-member');
    loadMembers();
  } else {
    showToast(res.error || 'Failed to update member', 'error');
  }
}

function promptDeleteMember(id, name, loanCount) {
  if (loanCount > 0) {
    showToast(`Cannot delete "${name}". They have ${loanCount} unreturned book(s).`, 'error');
    return;
  }
  document.getElementById('delete-member-id').value = id;
  document.getElementById('delete-member-name-display').textContent = `"${name}" (ID: ${id})`;
  openModal('modal-delete-member');
}

async function confirmDeleteMember() {
  const id = parseInt(document.getElementById('delete-member-id').value, 10);
  const res = await apiRequest(`/api/members/${id}`, { method: 'DELETE' });

  if (res.success) {
    showToast('Member removed', 'success');
    closeModal('modal-delete-member');
    loadMembers();
  } else {
    showToast(res.error || 'Failed to delete member', 'error');
  }
}

/* ==========================================================================
   4. ISSUE & RETURN
   ========================================================================== */

async function loadLoans() {
  const tbody = document.getElementById('transactions-table-body');
  if (tbody) {
    tbody.innerHTML = `<tr><td colspan="5" class="state-container"><div class="spinner"></div></td></tr>`;
  }

  const res = await apiRequest('/api/loans');
  if (!res.success) {
    showToast(res.error || 'Failed to load loans', 'error');
    return;
  }

  state.loans = res.loans || [];
  renderLoansTable();
}

function renderLoansTable() {
  const tbody = document.getElementById('transactions-table-body');
  if (!tbody) return;

  if (state.loans.length === 0) {
    tbody.innerHTML = `
      <tr>
        <td colspan="5">
          <div class="state-container">
            <div class="state-title">No Active Loans</div>
            <div class="state-desc">All books are in the library.</div>
            <button class="btn btn-gold btn-sm" onclick="openIssueModal()">Issue a Book</button>
          </div>
        </td>
      </tr>
    `;
    return;
  }

  tbody.innerHTML = state.loans.map(l => `
    <tr>
      <td><code>#${l.id}</code></td>
      <td><strong>${escapeHTML(l.book_title)}</strong> (ID: #${l.book_id})</td>
      <td>${escapeHTML(l.member_name)} (ID: #${l.member_id})</td>
      <td><span class="badge badge-warning">Active Loan</span></td>
      <td>
        <button class="btn btn-sm btn-gold" onclick="promptReturnBook(${l.id}, '${escapeHTML(l.book_title)}')">
          Return Book
        </button>
      </td>
    </tr>
  `).join('');
}

async function openIssueModal(preselectedBookId = null) {
  const [bRes, mRes] = await Promise.all([
    apiRequest('/api/books'),
    apiRequest('/api/members')
  ]);

  const books = (bRes.books || []).filter(b => b.available_copies > 0);
  const members = mRes.members || [];

  if (books.length === 0) {
    showToast('No copies available to issue.', 'warning');
    return;
  }

  const bSelect = document.getElementById('issue-book-select');
  const mSelect = document.getElementById('issue-member-select');

  bSelect.innerHTML = books.map(b => `
    <option value="${b.id}" ${b.id === preselectedBookId ? 'selected' : ''}>
      #${b.id} - ${escapeHTML(b.title)} (${b.available_copies} available)
    </option>
  `).join('');

  mSelect.innerHTML = members.map(m => `
    <option value="${m.id}">
      #${m.id} - ${escapeHTML(m.name)} (${escapeHTML(m.department)})
    </option>
  `).join('');

  openModal('modal-issue-book');
}

function quickIssueBook(bookId) {
  openIssueModal(bookId);
}

async function handleIssueSubmit(e) {
  e.preventDefault();
  const book_id = parseInt(document.getElementById('issue-book-select').value, 10);
  const member_id = parseInt(document.getElementById('issue-member-select').value, 10);

  const res = await apiRequest('/api/loans/issue', {
    method: 'POST',
    body: JSON.stringify({ book_id, member_id })
  });

  if (res.success) {
    showToast('Book issued successfully', 'success');
    closeModal('modal-issue-book');
    refreshCurrentView();
  } else {
    showToast(res.error || 'Failed to issue book', 'error');
  }
}

function promptReturnBook(loanId, title) {
  document.getElementById('return-tx-id').value = loanId;
  document.getElementById('return-book-title-display').textContent = `"${title}"`;
  openModal('modal-return-book');
}

async function confirmReturnBook() {
  const loan_id = parseInt(document.getElementById('return-tx-id').value, 10);
  const res = await apiRequest('/api/loans/return', {
    method: 'POST',
    body: JSON.stringify({ loan_id })
  });

  if (res.success) {
    showToast('Book returned successfully', 'success');
    closeModal('modal-return-book');
    refreshCurrentView();
  } else {
    showToast(res.error || 'Failed to return book', 'error');
  }
}

/* ==========================================================================
   RESET SAMPLE DATA
   ========================================================================== */

function promptResetDemo() {
  openModal('modal-reset-demo');
}

async function confirmResetDemo() {
  const res = await apiRequest('/api/reset-demo', { method: 'POST' });
  if (res.success) {
    showToast('Sample data reset successfully', 'success');
    closeModal('modal-reset-demo');
    refreshCurrentView();
  } else {
    showToast(res.error || 'Failed to reset data', 'error');
  }
}

/* ==========================================================================
   INITIALIZATION
   ========================================================================== */

function initEventListeners() {
  const mobileToggle = document.getElementById('mobile-menu-toggle');
  const sidebar = document.getElementById('sidebar');
  if (mobileToggle && sidebar) {
    mobileToggle.addEventListener('click', () => sidebar.classList.toggle('mobile-open'));
  }

  window.addEventListener('hashchange', () => {
    const h = window.location.hash.replace('#', '') || 'dashboard';
    navigateTo(h);
  });

  const bookSearch = document.getElementById('search-books-input');
  if (bookSearch) {
    bookSearch.addEventListener('input', (e) => {
      state.bookSearchQuery = e.target.value;
      renderBooksTable();
    });
  }

  const catSelect = document.getElementById('filter-book-category');
  if (catSelect) {
    catSelect.addEventListener('change', (e) => {
      state.bookCategoryFilter = e.target.value;
      loadBooks();
    });
  }

  const memSearch = document.getElementById('search-members-input');
  if (memSearch) {
    memSearch.addEventListener('input', (e) => {
      state.memberSearchQuery = e.target.value;
      loadMembers();
    });
  }
}

document.addEventListener('DOMContentLoaded', () => {
  initEventListeners();
  const init = window.location.hash.replace('#', '') || 'dashboard';
  navigateTo(init);
});
