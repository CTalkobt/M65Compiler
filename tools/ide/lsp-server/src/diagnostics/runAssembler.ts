import { spawnSync } from 'child_process';
import * as path from 'path';
import * as fs from 'fs';
import * as os from 'os';
import { parseDiagnostics } from './parseDiagnostics';
import { ParsedDiagnostic } from './diagnosticTypes';

const DEFAULT_CA45_PATH = 'bin/ca45';
const TIMEOUT_MS = 10000;

export function runAssembler(
  ca45Path: string,
  sourceFile: string,
  workspacePath: string
): ParsedDiagnostic[] {
  // Resolve ca45 path relative to workspace if not absolute
  const resolvedCa45 = path.isAbsolute(ca45Path)
    ? ca45Path
    : path.join(workspacePath, ca45Path);

  // Check if ca45 exists
  if (!fs.existsSync(resolvedCa45)) {
    return [
      {
        file: sourceFile,
        line: 1,
        column: 1,
        severity: 'error',
        message: `ca45 assembler not found at ${resolvedCa45}. Configure m65.ca45Path in settings.`,
      },
    ];
  }

  // Create a temporary output file
  const tempDir = os.tmpdir();
  const tempOutput = path.join(tempDir, `ca45_out_${Date.now()}.bin`);

  try {
    const result = spawnSync(resolvedCa45, [sourceFile, '-o', tempOutput], {
      cwd: workspacePath,
      encoding: 'utf-8',
      timeout: TIMEOUT_MS,
      stdio: ['ignore', 'pipe', 'pipe'],
    });

    // Clean up temp file if it was created
    try {
      fs.unlinkSync(tempOutput);
    } catch {
      // Ignore cleanup errors
    }

    if (result.error) {
      if ('code' in result.error && result.error.code === 'ETIMEDOUT') {
        return [
          {
            file: sourceFile,
            line: 1,
            column: 1,
            severity: 'error',
            message: `ca45 timed out after ${TIMEOUT_MS}ms`,
          },
        ];
      }
      return [
        {
          file: sourceFile,
          line: 1,
          column: 1,
          severity: 'error',
          message: `Failed to run ca45: ${result.error.message}`,
        },
      ];
    }

    // Parse stderr for diagnostics
    const stderr = result.stderr || '';
    const diagnostics = parseDiagnostics(stderr);

    // If we got diagnostics, return them
    if (diagnostics.length > 0) {
      return diagnostics;
    }

    // If exit code is non-zero but no diagnostics parsed, report generic error
    if (result.status !== 0) {
      return [
        {
          file: sourceFile,
          line: 1,
          column: 1,
          severity: 'error',
          message: `ca45 exited with status ${result.status}`,
        },
      ];
    }

    return [];
  } catch (err) {
    return [
      {
        file: sourceFile,
        line: 1,
        column: 1,
        severity: 'error',
        message: `Internal error running ca45: ${err instanceof Error ? err.message : String(err)}`,
      },
    ];
  }
}
