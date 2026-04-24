import * as vscode from 'vscode';

export class RslFileCoverage extends vscode.FileCoverage {
  constructor(uri: vscode.Uri, coverageData: readonly vscode.FileCoverageDetail[]) {
    let covered = new vscode.TestCoverageCount(0, 0);
    super(uri, covered);
  }
}