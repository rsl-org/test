import * as vscode from 'vscode';

import { log } from './logger';
import { TestRunner } from './controller/runner';


export async function find_test_binaries(): Promise<vscode.Uri[]> {
  const workspaceFolders = vscode.workspace.workspaceFolders;
  if (!workspaceFolders || workspaceFolders.length === 0) {
    return [];
  }

  const root = workspaceFolders[0].uri.fsPath;
  const pattern = new vscode.RelativePattern(
    root,
    "build/**/*-test"
  );

  return await vscode.workspace.findFiles(pattern, undefined, 1);
}

let test_suites: Map<vscode.Uri, TestRunner> = new Map<vscode.Uri, TestRunner>();

export async function activate(context: vscode.ExtensionContext) {
  log.init('rsl-test', 'log');
  log.info('Starting rsl-test extension');

  for (const runner_binary of await find_test_binaries()) {
    log.info(`Found test suite at ${runner_binary}`);
    const runner = new TestRunner(context, runner_binary);
    test_suites.set(runner_binary, runner);
    await runner.handle_discovery();
  }

  context.subscriptions.push(
    vscode.commands.registerCommand('rsl-test.runAll', async () => {
      for (const [uri, runner] of test_suites) {
        await runner.handle_discovery();
      }
      // TODO actually run all tests
      log.popup('Not implemented');
    }));

  if (test_suites) {
    log.popup(`${test_suites.size} rsl-test suite${test_suites.size === 1 ? '' : 's'} found.`);
  }
}

export async function deactivate() {
  for (const [uri, runner] of test_suites) {
    await runner.dispose()
  }
  test_suites.clear();
}
