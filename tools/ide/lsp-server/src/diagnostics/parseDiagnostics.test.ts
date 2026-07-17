import { strict as assert } from 'assert';
import { test, describe } from 'node:test';
import { parseDiagnosticLine, parseDiagnostics } from './parseDiagnostics';

describe('parseDiagnostics', () => {
  describe('parseDiagnosticLine', () => {
    test('parses single error line', () => {
      const line = 'test.c:1:18: error: Expected \';\'. Found \'bar\' (IDENTIFIER) instead.';
      const result = parseDiagnosticLine(line);

      assert.ok(result);
      assert.strictEqual(result!.file, 'test.c');
      assert.strictEqual(result!.line, 1);
      assert.strictEqual(result!.column, 18);
      assert.strictEqual(result!.severity, 'error');
      assert.strictEqual(result!.message, 'Expected \';\'. Found \'bar\' (IDENTIFIER) instead.');
    });

    test('parses warning line', () => {
      const line = 'test.s:5:1: warning: Unused variable x';
      const result = parseDiagnosticLine(line);

      assert.ok(result);
      assert.strictEqual(result!.severity, 'warning');
      assert.strictEqual(result!.message, 'Unused variable x');
    });

    test('parses note line', () => {
      const line = 'test.c:10:5: note: Variable declared here';
      const result = parseDiagnosticLine(line);

      assert.ok(result);
      assert.strictEqual(result!.severity, 'note');
    });

    test('handles file paths with slashes', () => {
      const line = 'src/main/test.c:42:1: error: Syntax error';
      const result = parseDiagnosticLine(line);

      assert.ok(result);
      assert.strictEqual(result!.file, 'src/main/test.c');
      assert.strictEqual(result!.line, 42);
    });

    test('handles absolute file paths', () => {
      const line = '/home/user/project/test.c:1:1: error: Error message';
      const result = parseDiagnosticLine(line);

      assert.ok(result);
      assert.strictEqual(result!.file, '/home/user/project/test.c');
    });

    test('returns null for non-matching line', () => {
      const result = parseDiagnosticLine('Random text that does not match');
      assert.strictEqual(result, null);
    });

    test('handles messages with colons', () => {
      const line = 'test.c:1:1: error: Expected \':\' in ternary operator: a ? b : c';
      const result = parseDiagnosticLine(line);

      assert.ok(result);
      assert.strictEqual(result!.message, 'Expected \':\' in ternary operator: a ? b : c');
    });
  });

  describe('parseDiagnostics', () => {
    test('parses multiple diagnostic lines', () => {
      const stderr = [
        'test.c:1:1: error: First error',
        'test.c:2:2: warning: First warning',
        'test.c:3:3: note: First note',
      ].join('\n');

      const results = parseDiagnostics(stderr);

      assert.strictEqual(results.length, 3);
      assert.strictEqual(results[0].severity, 'error');
      assert.strictEqual(results[1].severity, 'warning');
      assert.strictEqual(results[2].severity, 'note');
    });

    test('skips empty lines', () => {
      const stderr = [
        'test.c:1:1: error: First error',
        '',
        'test.c:2:2: error: Second error',
      ].join('\n');

      const results = parseDiagnostics(stderr);

      assert.strictEqual(results.length, 2);
      assert.strictEqual(results[0].line, 1);
      assert.strictEqual(results[1].line, 2);
    });

    test('catches unparseable lines as errors', () => {
      const stderr = [
        'test.c:1:1: error: Formatted error',
        'Segmentation fault',
      ].join('\n');

      const results = parseDiagnostics(stderr);

      assert.strictEqual(results.length, 2);
      assert.strictEqual(results[0].severity, 'error');
      assert.strictEqual(results[1].severity, 'error');
      assert.strictEqual(results[1].message, 'Segmentation fault');
      assert.strictEqual(results[1].line, 1);
    });

    test('handles empty stderr', () => {
      const results = parseDiagnostics('');
      assert.strictEqual(results.length, 0);
    });

    test('handles only whitespace', () => {
      const results = parseDiagnostics('   \n\n  \n');
      assert.strictEqual(results.length, 0);
    });
  });
});
