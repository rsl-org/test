import * as vscode from 'vscode';

class Logger {
  private out?: vscode.OutputChannel;

  public init(name: string, id: string) {
    this.out = vscode.window.createOutputChannel(name, id);
  }

  public emit_message(level: string, message: string) {
    const time = new Date().toISOString();
    const line = `${time} [${level}] ${message}`;
    if (!this.out) {
      console.error(line);
    } else {
      this.out.appendLine(line);
    }
  }

  public popup(message: string) {
    vscode.window.showInformationMessage(message);
  }

  public debug = (message: string) => this.emit_message('debug', message);
  public info = (message: string) => this.emit_message('info', message);
  public warning = (message: string) => this.emit_message('warning', message);
  public error = (message: string) => this.emit_message('error', message);
}

export const log = new Logger();