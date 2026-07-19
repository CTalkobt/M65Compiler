import { spawnSync } from 'child_process';
import * as path from 'path';
import * as fs from 'fs';
import * as os from 'os';
import { parseDiagnostics } from './parseDiagnostics';
import { ParsedDiagnostic } from './diagnosticTypes';

const DEFAULT_CC45_PATH = 'bin/cc45';
const TIMEOUT_MS = 10000;

export function runCompiler(
  cc45Path: string,
  sourceFile: string,
  workspacePath: string
): ParsedDiagnostic[] {
  // Resolve cc45 path relative to workspace if not absolute
  const resolvedCc45 = path.isAbsolute(cc45Path)
    ? cc45Path
    : path.join(workspacePath, cc45Path);

  // Check if cc45 exists
  if (!fs.existsSync(resolvedCc45)) {
    return [
      {
        file: sourceFile,
        line: 1,
        column: 1,
        severity: 'error',
        message: `cc45 compiler not found at ${resolvedCc45}. Configure m65.cc45Path in settings.`,
      },
    ];
  }

  // Create a temporary output file
  const tempDir = os.tmpdir();
  const tempOutput = path.join(tempDir, `cc45_out_${Date.now()}.s`);

  try {
    const result = spawnSync(resolvedCc45, [sourceFile, '-o', tempOutput], {
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
            message: `cc45 timed out after ${TIMEOUT_MS}ms`,
          },
        ];
      }
      return [
        {
          file: sourceFile,
          line: 1,
          column: 1,
          severity: 'error',
          message: `Failed to run cc45: ${result.error.message}`,
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
          message: `cc45 exited with status ${result.status}`,
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
        message: `Internal error running cc45: ${err instanceof Error ? err.message : String(err)}`,
      },
    ];
  }
}
