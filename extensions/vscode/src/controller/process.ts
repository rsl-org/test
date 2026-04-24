import * as vscode from 'vscode';
import { spawn, ChildProcessWithoutNullStreams } from 'child_process';
import { log } from "../logger";
import path from 'path';

export interface Message {
  action: string;
  [key: string]: any;
}

export class TestProcess {
  protected proc: ChildProcessWithoutNullStreams | undefined;
  private stdoutBuffer = '';

  protected handleUnsolicited(msg: Message) {
    log.warning(`unhandled message: ${msg.action}`);
  }

  private pendingRequests = new Map<
    string,
    { resolve: (msg: Message) => void; reject: (err: any) => void }
  >();

  constructor(public program: vscode.Uri) { }

  public run(args: string[] = []) {
    const basename = path.basename(this.program.fsPath);
    this.proc = spawn(this.program.fsPath, args, {
      stdio: ['pipe', 'pipe', 'pipe'],
      shell: true,
      cwd: path.dirname(this.program.fsPath)
    });

    this.proc.stdout.on('data', (chunk) => this.handle_stdout(chunk));
    this.proc.stderr.on('data', (chunk) => {
      log.error(`[${basename} stderr] ${chunk.toString()}`);
    });
    this.proc.on('exit', (code) => {
      if (code !== 0) {
        log.error(`Process ${basename} exited with code ${code}`);
      }
      // Reject all pending requests
      for (const { reject } of this.pendingRequests.values()) {
        reject(new Error('Process exited'));
      }
      this.pendingRequests.clear();
    });
  }

  public send_oneshot(action: string) {
    if (this.proc === undefined) {
      return;
    }
    const encoded = action + '\n';
    this.proc?.stdin.write(encoded);
  }

  public send(action: string, payload: any = {}) {
    if (this.proc === undefined) {
      return new Promise<Message>((resolve, reject) => {
        reject("Process not running");
      });
    }
    const message: Message = { action, ...payload };
    // const encoded = JSON.stringify(message) + '\n';
    const encoded = action + '\n';

    return new Promise<Message>((resolve, reject) => {
      this.proc?.stdin.write(encoded, (err) => {
        if (err) {
          this.pendingRequests.delete(action);
          reject(err);
        }
      });
      this.pendingRequests.set(action, { resolve, reject });
    });
  }

  private handle_stdout(chunk: Buffer) {
    this.stdoutBuffer += chunk.toString();

    let idx;
    while ((idx = this.stdoutBuffer.indexOf('\n')) !== -1) {
      const raw = this.stdoutBuffer.slice(0, idx);
      this.stdoutBuffer = this.stdoutBuffer.slice(idx + 1);

      if (raw.trim().length === 0) { continue; }

      let msg: Message;
      try {
        msg = JSON.parse(raw);
      } catch (e) {
        log.error('Failed to parse JSON:' + raw);
        continue;
      }

      this.dispatch(msg);
    }
  }

  private dispatch(msg: Message) {
    if (this.pendingRequests.has(msg.action)) {
      const handler = this.pendingRequests.get(msg.action)!;
      this.pendingRequests.delete(msg.action);
      handler.resolve(msg);
    } else {
      this.handleUnsolicited(msg);
    }
  }

  public async terminate() {
    if (!this.proc || this.proc.exitCode !== null) { return; }
    this.send_oneshot("exit");

    const exited = await Promise.race([
      new Promise<boolean>(resolve =>
        this.proc!.once("exit", () => resolve(true))
      ),
      new Promise<boolean>(resolve =>
        setTimeout(() => resolve(false), 1500) // TODO make graceful exit timeout configurable
      )
    ]);

    if (!exited) {
      // didn't exit gracefully in time, kill it
      this.proc.kill();
    }

    this.proc = null!;
  }

  public async dispose() {
    await this.terminate();
  }
}
