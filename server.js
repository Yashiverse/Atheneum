/**
 * server.js - Streamlined Node.js Server for Library Management System
 * Zero external dependencies (uses built-in http, fs, path, child_process, url).
 */

const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');
const url = require('url');

const PORT = 8080;
const BASE_DIR = __dirname;
const FRONTEND_DIR = path.join(BASE_DIR, 'frontend');
const BACKEND_DIR = path.join(BASE_DIR, 'backend');

const isWin = process.platform === 'win32';
const EXE_NAME = isWin ? 'library_core.exe' : 'library_core';
const EXE_PATH = path.join(BACKEND_DIR, EXE_NAME);

function ensureCBinary() {
    if (fs.existsSync(EXE_PATH)) return true;
    console.log(`[*] Compiling ${EXE_NAME} with gcc...`);
    const srcPath = path.join(BACKEND_DIR, 'library_core.c');
    try {
        const compile = spawnSync('gcc', ['-Wall', '-Wextra', '-O2', srcPath, '-o', EXE_PATH], { cwd: BASE_DIR });
        return compile.status === 0;
    } catch (e) {
        return false;
    }
}

function callC(action, payload) {
    const args = ['--api', action];
    if (payload !== undefined && payload !== null) {
        args.push('--data', typeof payload === 'object' ? JSON.stringify(payload) : String(payload));
    }
    try {
        const res = spawnSync(EXE_PATH, args, { cwd: BASE_DIR, encoding: 'utf-8', timeout: 10000 });
        const out = (res.stdout || '').trim();
        if (!out) return { success: false, error: res.stderr || 'Empty response from C core' };
        return JSON.parse(out);
    } catch (err) {
        return { success: false, error: err.message };
    }
}

const MIME_TYPES = {
    '.html': 'text/html; charset=utf-8',
    '.css': 'text/css; charset=utf-8',
    '.js': 'application/javascript; charset=utf-8',
    '.json': 'application/json; charset=utf-8',
    '.png': 'image/png',
    '.ico': 'image/x-icon'
};

function sendJSON(res, data, statusCode = 200) {
    const body = JSON.stringify(data);
    res.writeHead(statusCode, {
        'Content-Type': 'application/json; charset=utf-8',
        'Content-Length': Buffer.byteLength(body),
        'Access-Control-Allow-Origin': '*',
        'Access-Control-Allow-Methods': 'GET, POST, PUT, DELETE, OPTIONS',
        'Access-Control-Allow-Headers': 'Content-Type',
        'Cache-Control': 'no-cache, no-store, must-revalidate'
    });
    res.end(body);
}

function serveStatic(req, res, pathname) {
    let safePath = pathname === '/' ? '/index.html' : pathname;
    const filePath = path.join(FRONTEND_DIR, safePath);

    fs.stat(filePath, (err, stats) => {
        if (err || !stats.isFile()) {
            res.writeHead(404, { 'Content-Type': 'text/plain' });
            return res.end('404 Not Found');
        }
        const ext = path.extname(filePath).toLowerCase();
        res.writeHead(200, {
            'Content-Type': MIME_TYPES[ext] || 'application/octet-stream',
            'Content-Length': stats.size,
            'Access-Control-Allow-Origin': '*'
        });
        fs.createReadStream(filePath).pipe(res);
    });
}

const server = http.createServer((req, res) => {
    const parsedUrl = url.parse(req.url, true);
    const pathname = parsedUrl.pathname;
    const query = parsedUrl.query;

    if (req.method === 'OPTIONS') {
        res.writeHead(200, {
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, POST, PUT, DELETE, OPTIONS',
            'Access-Control-Allow-Headers': 'Content-Type'
        });
        return res.end();
    }

    let bodyData = '';
    req.on('data', chunk => { bodyData += chunk; });
    req.on('end', () => {
        let body = {};
        if (bodyData) {
            try { body = JSON.parse(bodyData); } catch (e) {}
        }

        if (req.method === 'GET' && pathname === '/api/dashboard') {
            return sendJSON(res, callC('get_dashboard'));
        }
        if (req.method === 'GET' && pathname === '/api/books') {
            return sendJSON(res, callC('list_books', { query: query.query || '', category: query.category || '' }));
        }
        if (req.method === 'POST' && pathname === '/api/books') {
            const r = callC('add_book', body);
            return sendJSON(res, r, r.success ? 200 : 400);
        }
        if (req.method === 'PUT' && pathname.startsWith('/api/books/')) {
            body.id = parseInt(pathname.split('/')[3], 10);
            const r = callC('edit_book', body);
            return sendJSON(res, r, r.success ? 200 : 400);
        }
        if (req.method === 'DELETE' && pathname.startsWith('/api/books/')) {
            const id = parseInt(pathname.split('/')[3], 10);
            const r = callC('delete_book', { id });
            return sendJSON(res, r, r.success ? 200 : 400);
        }

        if (req.method === 'GET' && pathname === '/api/members') {
            return sendJSON(res, callC('list_members', { query: query.query || '' }));
        }
        if (req.method === 'POST' && pathname === '/api/members') {
            const r = callC('add_member', body);
            return sendJSON(res, r, r.success ? 200 : 400);
        }
        if (req.method === 'PUT' && pathname.startsWith('/api/members/')) {
            body.id = parseInt(pathname.split('/')[3], 10);
            const r = callC('edit_member', body);
            return sendJSON(res, r, r.success ? 200 : 400);
        }
        if (req.method === 'DELETE' && pathname.startsWith('/api/members/')) {
            const id = parseInt(pathname.split('/')[3], 10);
            const r = callC('delete_member', { id });
            return sendJSON(res, r, r.success ? 200 : 400);
        }

        if (req.method === 'GET' && (pathname === '/api/loans' || pathname === '/api/transactions')) {
            return sendJSON(res, callC('list_loans'));
        }
        if (req.method === 'POST' && (pathname === '/api/loans/issue' || pathname === '/api/transactions/issue')) {
            const r = callC('issue_book', body);
            return sendJSON(res, r, r.success ? 200 : 400);
        }
        if (req.method === 'POST' && (pathname === '/api/loans/return' || pathname === '/api/transactions/return')) {
            const r = callC('return_book', body);
            return sendJSON(res, r, r.success ? 200 : 400);
        }
        if (req.method === 'POST' && pathname === '/api/reset-demo') {
            return sendJSON(res, callC('reset_demo'));
        }

        if (pathname.startsWith('/api/')) {
            return sendJSON(res, { success: false, error: 'Not Found' }, 404);
        }

        serveStatic(req, res, pathname);
    });
});

ensureCBinary();
server.listen(PORT, () => {
    console.log('='.repeat(60));
    console.log('   LIBRARY MANAGEMENT SYSTEM (Node.js & C Core)');
    console.log('='.repeat(60));
    console.log(`[*] Web Interface: http://localhost:${PORT}`);
    console.log(`[*] C Core Binary: ${EXE_PATH}`);
    console.log('='.repeat(60));
});
