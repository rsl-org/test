import * as vscode from 'vscode';
import { RslFileCoverage } from '../coverage/coverage';
import { log } from "../logger";
import { Message, TestProcess } from './process';
import path from 'path';
import { Protocol } from './protocol';


interface Watch {
  request: vscode.TestRunRequest;
  token: vscode.CancellationToken;
}

export class TestRunner extends TestProcess {
  private controller: vscode.TestController;
  private proc_counter: number = 0;
  private watches: Watch[] = [];

  constructor(context: vscode.ExtensionContext, private runner_path: vscode.Uri) {
    super(runner_path);

    this.controller = vscode.tests.createTestController('rslTestController', `rsl-test: ${path.basename(this.runner_path.fsPath)}`);
    context.subscriptions.push(this.controller);

    this.controller.refreshHandler = async () => {
      log.warning('Refresh');
    };

    this.controller.createRunProfile(
      'Run Tests', vscode.TestRunProfileKind.Run,
      (request, token) => this.handle_test(request, token), true, undefined, true);

    this.controller.createRunProfile(
      'Debug Tests', vscode.TestRunProfileKind.Debug,
      (request, token) => log.info('Debug request'), true);

    this.controller.createRunProfile(
      'Cover Tests', vscode.TestRunProfileKind.Coverage,
      (request, token) => this.handle_coverage(request, token), true, undefined, true);
  }

  async dispose() {
    await super.dispose();
    this.controller.dispose();
  }

  private start_process() {
    if (this.proc_counter++ === 0) {
      log.debug("Starting test process");
      const args: string[] = []; // ["--interactive"]
      this.run(args);
    }
  }

  private stop_process() {
    if (this.proc_counter-- <= 0) {
      log.debug("Stopping test process");
      this.proc_counter = 0;
      super.terminate();
    }
  }

  handleUnsolicited(msg: Message) {
    if (msg.action === "test_result" && Protocol.isTestResult(msg)) {
      for (const result of msg.results) {
        log.debug(`${result.name} ${result.outcome}`);
      }
    } else {
      log.debug(`Unsolicited message received: ${msg.action}`);
    }
  }

  async enable_continuous_testing(request: vscode.TestRunRequest, token: vscode.CancellationToken) {
    const watch: Watch = { request, token };
    this.watches.push(watch);
    log.info("Continuous testing enabled");

    this.start_process();
    if (request.include) {
      for (const inc of request.include ?? []) {
        log.info("test - " + inc.uri + " " + inc.label);
      }
    } else {
      // want all
      log.info("test all");
    }

    token.onCancellationRequested(() => {
      log.info("Continuous testing disabled");
      this.watches = this.watches.filter(w => w !== watch);
      if (request.include) {
        for (const inc of request.include ?? []) {
          log.info("/test - " + inc.uri + " " + inc.label);
        }
      } else {
        // want all
        log.info("/test all");
      }
      this.stop_process();
    });
  }

  async enable_coverage(request: vscode.TestRunRequest, token: vscode.CancellationToken) {
    log.info("Covering");
    this.start_process();
    // await this.send("cover");
    token.onCancellationRequested(() => {
      log.info("Stop covering");
      this.stop_process();
    });
  }

  async handle_discovery() {
    log.warning("Discovery started");

    this.start_process();
    const data = await super.send("list");
    this.stop_process();

    if (!Protocol.isListTests(data)) {
      return;
    }

    // TODO reset old tree
    for (const t of data.tests) {
      log.debug(t.name + " " + t.path + " " + t.full_name);
      // TODO use better test id
      const item = this.controller.createTestItem(t.name, t.name, vscode.Uri.file(t.path));

      let current = this.controller.items;
      for (const part of t.full_name.slice(0, -1)) {
        let child = current.get(part);
        if (!child) {
          log.debug("Inserting namespace " + part);
          child = this.controller.createTestItem(part, part);
          current.add(child);
        }
        current = child.children;
      }
      current.add(item);
    };
  }

  async handle_coverage(request: vscode.TestRunRequest, token: vscode.CancellationToken) {
    if (request.continuous) {
      // await this.runner.want_coverage(request, token);
      return;
    }
    // TODO enable coverage
    log.info("Running with coverage");
    this.handle_test(request, token);
  }

  async handle_test(request: vscode.TestRunRequest, token: vscode.CancellationToken) {
    if (request.continuous) {
      this.enable_continuous_testing(request, token);
      return;
    }

    const run = this.controller.createTestRun(request);
    for (const inc in request.include) {
      log.warning("test - " + inc.toString());
    }
    this.send_oneshot("run")

    const queue = new Set<vscode.TestItem>();

    if (request.include) {
      request.include.forEach(test => queue.add(test));
    } else {
      this.controller.items.forEach(test => queue.add(test));
    }

    for (const test of queue) {
      if (token.isCancellationRequested) {
        run.end();
        return;
      }

      run.started(test);
      log.warning("Running test: " + test.uri?.toString() + " " + test.label + " continuous: " + request.continuous);

      run.passed(test);
      run.appendOutput("test output", undefined, test);
    }
    run.end();
  }
}

export async function debugTest(testId: string) {
  await vscode.debug.startDebugging(undefined, {
    type: 'rsl-test',
    request: 'launch',
    name: `Debug Test: ${testId}`,
    testId: testId
  });
}