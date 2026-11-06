// Runs the LSP server's tests (src/**/*.test.ts) with node's test runner and tsx.
// The files are found here: `node --test` expands a glob only from Node 21,
// and the package supports Node 18 (CI tests 18.x and 20.x).
import { readdirSync } from 'node:fs';
import { join } from 'node:path';
import { spawnSync } from 'node:child_process';

function find(dir) {
    const out = [];
    for (const e of readdirSync(dir, { withFileTypes: true })) {
        const p = join(dir, e.name);
        if (e.isDirectory()) out.push(...find(p));
        else if (e.name.endsWith('.test.ts')) out.push(p);
    }
    return out;
}

const files = find('src').sort();
if (files.length === 0) {
    console.error('no tests found under src/');
    process.exit(1);
}
const r = spawnSync(process.execPath, ['--import', 'tsx', '--test', ...files], { stdio: 'inherit' });
process.exit(r.status ?? 1);
